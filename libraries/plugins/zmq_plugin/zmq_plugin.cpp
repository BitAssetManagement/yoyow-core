/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/zmq_plugin/zmq_plugin.hpp>
#include <graphene/zmq_plugin/zmq_api.hpp>

#include <graphene/chain/impacted.hpp>
#include <graphene/chain/config.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <zmq.hpp>

namespace {
  const char* SENDER_BIND_OPT = "zmq-sender-bind";
  const char* SENDER_BIND_DEFAULT = "tcp://127.0.0.1:5556";
  const char* WHITELIST_OPT = "zmq-whitelist-account";
  
  const int32_t MSGTYPE_ACTION_TRACE = 0;
  const int32_t MSGTYPE_IRREVERSIBLE_BLOCK = 1;
  const int32_t MSGTYPE_FORK = 2;
  const int32_t MSGTYPE_ACCEPTED_BLOCK = 3;
  const int32_t MSGTYPE_FAILED_TX = 4;
  const int32_t MSGTYPE_BALANCE_RESOURCE = 5;
}

namespace graphene { namespace zmq_plugin {


class zmq_plugin_impl
{
   public:
        zmq_plugin&     _self;
        zmq::context_t  context;
        zmq::socket_t   sender_socket;
        string          socket_bind_str;
        uint32_t        _end_block = 0;
        size_t          asset_cache_size = 10;


        zmq_plugin_impl(zmq_plugin& _plugin): 
            _self( _plugin ), 
            context(1),
            sender_socket(context, ZMQ_PUSH){}
        ~zmq_plugin_impl(){}

        graphene::chain::database& database()
        {
            return _self.database();
        }

        void send_msg( const string content, int32_t msgtype, int32_t msgopts)
        {
            zmq::message_t message(content.length()+sizeof(msgtype)+sizeof(msgopts));
            unsigned char* ptr = (unsigned char*) message.data();
            memcpy(ptr, &msgtype, sizeof(msgtype));
            ptr += sizeof(msgtype);
            memcpy(ptr, &msgopts, sizeof(msgopts));
            ptr += sizeof(msgopts);
            memcpy(ptr, content.c_str(), content.length());
            sender_socket.send(message);
        }

        template<typename X>
        bool contains( const operation& op ) const{
            return op.which() == operation::tag<X>::value;
        }

        void pruge_asset_cache(){
            const auto&  assets_cache = database().get_index_type<asset_cache_index>().indices().get<by_last_modify>();
            if( assets_cache.size() < asset_cache_size ) return ;

            auto itr = assets_cache.begin();
            if( itr != assets_cache.end() ){
                database().remove(*itr);
            }
        }

        asset_object get_asset_for_assets_cache( asset_aid_type asset_id ){
            asset_object result;
            const auto&  assets_cache = database().get_index_type<asset_cache_index>().indices().get<by_aid>();
            auto as = assets_cache.find(asset_id);

            if( as != assets_cache.end() ){
                database().modify<assets_cache_object>( *as , [&](assets_cache_object& ac ){
                    ac.last_modify = fc::time_point::now();
                });
                return as->ao;
            } else {
                auto ass = database().get_asset_by_aid(asset_id);
                auto test = database().create<assets_cache_object>( [&](assets_cache_object& ac ){
                    ac.asset_id = asset_id;
                    ac.ao = ass;
                    ac.last_modify = fc::time_point::now();
                });
                // ilog("${log}",("log",test.id));
                pruge_asset_cache();
                return ass;
            }

            return result;
        }


        uint64_t get_precision( uint8_t decimals ) const{
            uint64_t p10 = 1;
            uint64_t p = decimals;
            while( p > 0  ) {
                p10 *= 10; --p;
            }
            return p10;
        }

        string get_asset_string( asset_object ao, asset balance ){
            auto precision = get_precision(ao.precision);
            auto amount = balance.amount.value;
            string sign = amount < 0 ? "-" : "";
            int64_t abs_amount = std::abs(amount);
            string result = fc::to_string( static_cast<int64_t>(abs_amount) / precision );
            if( ao.precision )
            {
                auto fract = static_cast<int64_t>(abs_amount) % precision;
                result += "." + fc::to_string(precision + fract).erase(0,1);
            }
            return sign + result + " " + ao.symbol;
        }

        currency_balance get_currency_balance( account_uid_type uuid, asset_aid_type asset_id, asset balance ){
            auto ao = get_asset_for_assets_cache( asset_id );
            auto balance_str = get_asset_string( ao, balance );
            string prepaid = "0.00000 " + ao.symbol;
            share_type csaf = 0;
            if( asset_id == 0 ){
                auto aco = database().get_account_statistics_by_uid(uuid);
                auto yoyo_ao = get_asset_for_assets_cache(0);
                prepaid = get_asset_string(yoyo_ao, asset(aco.prepaid,0));
                csaf = aco.csaf;
            }
            return currency_balance{ uuid, ao.issuer, balance_str, prepaid, csaf};
        }

        void on_accepted_block(const signed_block& b){
            auto block_num = b.block_num();
            if( block_num <= _end_block ){
                // 分叉信号
                zmq_fork_block_object zfbo;
                zfbo.invalid_block_num = block_num;
                send_msg(fc::json::to_string(zfbo), MSGTYPE_FORK, 0);
            }
            
            _end_block = block_num;

            zmq_block_object zbo;
            //  transaction 并解析。
            for(auto& trx : b.transactions){
                for(auto& operation : trx.operations){
                    // 解析 operation ,并获取余额。
                    on_operation_trace( zbo, operation, b );
                }
            }

            zbo.block = b;
            zbo.last_irreversible_block = database().get_dynamic_global_properties().last_irreversible_block_num;

            send_msg(fc::json::to_string(zbo), MSGTYPE_ACCEPTED_BLOCK, 0);
        }

        void on_operation_trace(zmq_block_object& zbo,  const operation& op, const signed_block& b){
            std::set<account_uid_type> accounts;
            assetmoves asset_moves;
            find_account_and_tokens( op, accounts, asset_moves );


            // 获取余额
            for(auto asset_itr = asset_moves.begin(); asset_itr != asset_moves.end(); asset_itr++){
                auto asset_id = asset_itr->first;
                for(auto acc_itr = asset_itr->second.begin(); acc_itr != asset_itr->second.end(); acc_itr++){
                    auto acc = *acc_itr;
                    // ilog("${acc} ${ass}",("acc",acc)("ass",asset_id));
                    try{
                        zbo.currency_balances.emplace_back( get_currency_balance(acc, asset_id, database().get_balance( acc, asset_id)) );
                    } catch( fc::exception e) {
                        elog("get asset wrong ${asset_id} details: ${error}",("asset_id",asset_id)("error",e.to_detail_string()));
                    } catch( ... ){
                        elog("get asset wrong ${asset_id}",("asset_id",asset_id));
                    }
                }
            }

            // send_msg(fc::json::to_string(zoo), MSGTYPE_ACTION_TRACE, 0);
        }

        void find_account_and_tokens( const operation& op, std::set<account_uid_type>& accounts, assetmoves& asset_moves ){
            if( contains<transfer_operation>( op )){
                auto result_op = op.get<transfer_operation>();
                asset_moves[result_op.amount.asset_id].insert(result_op.from);
                asset_moves[result_op.amount.asset_id].insert(result_op.to);
            } else if( contains<account_update_proxy_operation>( op )){

            } else if( contains<override_transfer_operation>( op )){
                auto result_op = op.get<override_transfer_operation>();
                asset_moves[result_op.amount.asset_id].insert(result_op.from);
                asset_moves[result_op.amount.asset_id].insert(result_op.to);
            } else if( contains<account_enable_allowed_assets_operation>( op )){

            } else if( contains<account_update_allowed_assets_operation>( op )){

            }
        }

        void on_pending_transaction(const signed_transaction& trx){
            send_msg(fc::json::to_string(trx), MSGTYPE_ACTION_TRACE, 0);
        }


        string get_accounts_balances(const std::vector<asset_aid_type>& assets_id){
            zmq_accounts_info_object zai;
            const auto& idx = database().get_index_type<account_index>().indices().get<by_id>();
            
            ilog("get_accounts_balances begin");
            int counter = 0;
            for(auto itr = idx.begin(); itr != idx.end(); itr++){
                // ilog("${uid} ${name}",("uid",itr->uid)("name",itr->name));
                send_balances_by_account(itr->uid, assets_id);
                counter++;
            }
            ilog("get_accounts_balances end ${count}",("count",counter));
            return "{\"result\":\"OK\"}";
        }

        string get_account_balances(const account_uid_type owner, const std::vector<asset_aid_type>& assets_id){
            send_balances_by_account(owner, assets_id);
            return "{\"result\":\"OK\"}";
        }

        void send_balances_by_account(const account_uid_type owner, const std::vector<asset_aid_type>& assets_id){
            zmq_accounts_info_object zai;
            auto& index = database().get_index_type<account_balance_index>().indices().get<by_account_asset>();
            int counter = 1;
            for( auto asset_id = assets_id.begin(); asset_id != assets_id.end(); asset_id++){
                auto itr = index.find(boost::make_tuple(owner, *asset_id));
                if( itr != index.end()){
                    zai.currency_balances.emplace_back( get_currency_balance( owner, *asset_id, itr->get_balance() ) );
                    if( counter%100 ==0 ){
                        send_msg(fc::json::to_string(zai), MSGTYPE_BALANCE_RESOURCE, 0);
                        zai.currency_balances.clear();
                    }
                }
                
            }

            if( zai.currency_balances.size() != 0){
                send_msg(fc::json::to_string(zai), MSGTYPE_BALANCE_RESOURCE, 0);
                zai.currency_balances.clear();
            }
        }
};

zmq_plugin::zmq_plugin() :
   my( new zmq_plugin_impl(*this) )
{
}

zmq_plugin::~zmq_plugin()
{
}

std::string zmq_plugin::plugin_name()const
{
   return "zmq_plugin";
}

void zmq_plugin::plugin_set_program_options(
   boost::program_options::options_description& cli,
   boost::program_options::options_description& cfg
   )
{
   cfg.add_options()
         (SENDER_BIND_OPT, boost::program_options::value<string>()->default_value(SENDER_BIND_DEFAULT),"ZMQ Sender Socket binding")
         ("zmq-block-start", boost::program_options::value<uint32_t>()->default_value(0), "get block after block-start")
         ("zmq-asset-cache-size", boost::program_options::value<size_t>()->default_value(10), "asset cahce size")
         ;
   cli.add(cfg);
}

void zmq_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    ilog("zmq plugin init.");
    my->sender_socket.bind(options.at(SENDER_BIND_OPT).as<string>());
    my->asset_cache_size = options.at("zmq-asset-cache-size").as<size_t>();
    // ilog("${asset_cache_size}",("asset_cache_size",my->asset_cache_size));
    uint32_t block_num_start = options.at("zmq-block-start").as<uint32_t>();

    database().add_index< primary_index< asset_cache_index > >();my->pruge_asset_cache();
    database().applied_block.connect( [this,block_num_start]( const signed_block& b){ 
        if( b.block_num() >= block_num_start )
            my->on_accepted_block(b); 
    } );
    // 因所有 operation 的结果都存在 block 里， 所以无需 on_pending_transaction 信号
    // database().on_pending_transaction.connect( [&]( const signed_transaction& trx){ my->on_pending_transaction(trx); } );
}

void zmq_plugin::plugin_startup()
{
    ilog("zmq plugin begin.");
}

void zmq_plugin::plugin_shutdown()
{
    ilog("zmq plugin end.");
}

std::string zmq_plugin::get_accounts_balances(const std::vector<asset_aid_type>& assets_id){
    return my->get_accounts_balances(assets_id);
}

std::string zmq_plugin::get_account_balances(const account_uid_type owner, const std::vector<asset_aid_type>& assets_id){
    return my->get_account_balances(owner, assets_id);
}

} }


// FC_API(graphene::zmq_plugin::zmq_plugin_impl,(hi))