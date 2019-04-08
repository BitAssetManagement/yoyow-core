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

        void on_accepted_block(const signed_block& b){
            auto block_num = b.block_num();
            if( block_num <= _end_block ){
                // 分叉信号
                zmq_fork_block_object zfbo;
                zfbo.invalid_block_num = block_num;
                send_msg(fc::json::to_string(zfbo), MSGTYPE_FORK, 0);
            }
            
            _end_block = block_num;

            // 块信息
            {
                zmq_accepted_block_object zabo;
                zabo.accepted_block_num = block_num;
                zabo.accepted_block_timestamp = b.timestamp;
                zabo.accepted_block_witness = b.witness;
                zabo.accepted_witness_signature = b.witness_signature;
                send_msg(fc::json::to_string(zabo), MSGTYPE_ACCEPTED_BLOCK, 0);
            }

            //  transaction 并解析。
            for(auto& trx : b.transactions){
                for(auto& operation : trx.operations){
                    // 解析 operation ,并获取余额。
                    on_operation_trace( operation, b, trx.id());
                }
            }
        }

        void on_operation_trace( const operation& op, const signed_block& b, transaction_id_type trx_id ){
            zmq_operation_object zoo;
            std::set<account_uid_type> accounts;
            assetmoves asset_moves;
            find_account_and_tokens( op, accounts, asset_moves );


            // 获取余额
            for(auto asset_itr = asset_moves.begin(); asset_itr != asset_moves.end(); asset_itr++){
                auto asset_id = asset_itr->first;
                for(auto acc_itr = asset_itr->second.begin(); acc_itr != asset_itr->second.end(); acc_itr++){
                    auto acc = *acc_itr;
                    // ilog("${acc} ${ass}",("acc",acc)("ass",asset_id));
                    zoo.currency_balances.emplace_back(currency_balance{ acc, database().get_balance(*acc_itr, asset_id) });
                }
            }
            zoo.operation_trace = op;
            zoo.block_num = b.block_num();
            zoo.block_time = b.timestamp;
            zoo.trx_id = trx_id;
            send_msg(fc::json::to_string(zoo), MSGTYPE_ACTION_TRACE, 0);
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
                ilog("${uid} ${name}",("uid",itr->uid)("name",itr->name));
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
                    zai.currency_balances.emplace_back(currency_balance{ owner, itr->get_balance() });
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
//    cli.add_options()
//          ("track-account", boost::program_options::value<string>()->default_value("[]"), "Account ID to track history for (specified as a JSON array)")
//          ("partial-operations", boost::program_options::value<bool>(), "Keep only those operations in memory that are related to account history tracking")
//          ("max-ops-per-account", boost::program_options::value<uint32_t>(), "Maximum number of operations per account will be kept in memory")
//          ;
//    cfg.add(cli);
}

void zmq_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    ilog("zmq plugin init.");
    my->sender_socket.bind(SENDER_BIND_DEFAULT);
    my->database();
    database().applied_block.connect( [&]( const signed_block& b){ my->on_accepted_block(b); } );
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