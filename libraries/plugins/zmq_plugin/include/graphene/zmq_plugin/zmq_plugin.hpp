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
#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>

#include <graphene/chain/operation_history_object.hpp>

#include <fc/thread/future.hpp>

namespace graphene { namespace zmq_plugin {
   using namespace chain;
   
typedef std::map<asset_aid_type,std::set<account_uid_type>> assetmoves;

struct zmq_transaction : public processed_transaction
{
   zmq_transaction( const processed_transaction& trx ): processed_transaction(trx){
      trx_id = id();
   }

   transaction_id_type trx_id;
};

struct zmq_block : public signed_block {
   zmq_block(){}
   zmq_block( const signed_block block ): signed_block( block ){
      block_id = id();
      block_id_num = block_num();
      trxs.reserve( transactions.size() );
      for( const processed_transaction& tx : transactions )
         trxs.push_back( tx );
      transactions.clear();
   }
   zmq_block( const zmq_block& block ) = default;

   block_id_type              block_id;
   uint32_t                   block_id_num;
   vector< zmq_transaction >  trxs;
};

struct currency_balance {
   account_uid_type           account_name;
   account_uid_type           issuer;
   string                     balance;
   string                     prepaid;
   share_type                 csaf;
   bool                       deleted = false;

   currency_balance(account_uid_type _account_name, account_uid_type _issuer, string _balance, string _prepaid, share_type _csaf):
      account_name(_account_name),issuer(_issuer),balance(_balance),prepaid(_prepaid),csaf(_csaf){
   }
};

struct zmq_operation_object {
   uint32_t                      block_num;
   fc::time_point_sec            block_time;
   transaction_id_type           trx_id;
   operation                     operation_trace;
   vector<currency_balance>      currency_balances;
   uint32_t                      last_irreversible_block;
};

struct zmq_fork_block_object {
   uint32_t             invalid_block_num;
};
struct zmq_block_object {
   zmq_block                     block;
   vector<currency_balance>      currency_balances;
   uint32_t                      last_irreversible_block;
};

struct zmq_accounts_info_object {
   vector<currency_balance>     currency_balances;
};


class assets_cache_object : public abstract_object<assets_cache_object>
{
   public:
      static const uint8_t space_id = protocol_ids;
      static const uint8_t type_id  = assets_cache_object_type;

      asset_aid_type asset_id;
      asset_object ao;
      fc::time_point_sec last_modify;// = now();
};

struct by_aid;
struct by_last_modify;
typedef multi_index_container<
   assets_cache_object,
   indexed_by<
      ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
      ordered_unique< tag<by_aid>, member< assets_cache_object, asset_aid_type, &assets_cache_object::asset_id > >,
      ordered_non_unique< tag<by_last_modify>, member<assets_cache_object, fc::time_point_sec, &assets_cache_object::last_modify> >
   >
> assets_cache_multi_index_type;
typedef generic_index<assets_cache_object, assets_cache_multi_index_type> asset_cache_index;

class zmq_plugin_impl;

class zmq_plugin : public graphene::app::plugin
{
   public:
        zmq_plugin();
        virtual ~zmq_plugin();

        std::string plugin_name()const override;
        virtual void plugin_set_program_options(
        boost::program_options::options_description& cli,
        boost::program_options::options_description& cfg) override;
        virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
        virtual void plugin_startup() override;
        virtual void plugin_shutdown() override;

        std::string get_accounts_balances(const std::vector<asset_aid_type>& assets_id);
        std::string get_account_balances(const account_uid_type owner, const std::vector<asset_aid_type>& assets_id);

        friend class zmq_plugin_impl;
        std::unique_ptr<zmq_plugin_impl> my;
};

} } //graphene::zmq_plugin

FC_REFLECT_DERIVED( graphene::zmq_plugin::zmq_transaction,(graphene::chain::processed_transaction),
            (trx_id) )

FC_REFLECT_DERIVED( graphene::zmq_plugin::zmq_block,(graphene::chain::signed_block),
            (block_id)(block_id_num)(trxs) )

FC_REFLECT( graphene::zmq_plugin::currency_balance,
            (account_name)(issuer)(balance)(prepaid)(csaf)(deleted) )

FC_REFLECT( graphene::zmq_plugin::zmq_operation_object,
            (block_num)(block_time)(trx_id)(operation_trace)(currency_balances)(last_irreversible_block) )

FC_REFLECT( graphene::zmq_plugin::zmq_fork_block_object,
            (invalid_block_num) )

FC_REFLECT( graphene::zmq_plugin::zmq_block_object,
            (block)(currency_balances)(last_irreversible_block) )

FC_REFLECT( graphene::zmq_plugin::zmq_accounts_info_object,
            (currency_balances) )

FC_REFLECT_DERIVED( graphene::zmq_plugin::assets_cache_object,(graphene::db::object),
            (asset_id)(ao)(last_modify) )
