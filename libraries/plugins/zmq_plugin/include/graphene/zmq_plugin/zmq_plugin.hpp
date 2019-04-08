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
   //using namespace graphene::db;
   //using boost::multi_index_container;
   //using namespace boost::multi_index;

typedef std::map<asset_aid_type,std::set<account_uid_type>> assetmoves;

struct currency_balance {
   account_uid_type           account_name;
   asset                      balance;
   bool                       deleted = false;

   currency_balance(account_uid_type _account_name, asset _balance):
      account_name(_account_name),balance(_balance){
   }
};

struct zmq_operation_object {
   uint32_t                      block_num;
   fc::time_point_sec            block_time;
   transaction_id_type           trx_id;
   operation                     operation_trace;
   // vector<resource_balance>      resource_balances;
   // vector<voter_info>            voter_infos;
   vector<currency_balance>      currency_balances;
   uint32_t                      last_irreversible_block;
};

struct zmq_fork_block_object {
   uint32_t             invalid_block_num;
};

struct zmq_accepted_block_object {
   uint32_t             accepted_block_num;
   fc::time_point_sec   accepted_block_timestamp;
   account_uid_type     accepted_block_witness;
   signature_type       accepted_witness_signature;
};

struct zmq_accounts_info_object {
   // vector<voter_info>           voter_infos;
   vector<currency_balance>     currency_balances;
};

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


// FC_REFLECT( zmqplugin::zmq_action_object,
//             (global_action_seq)(block_num)(block_time)(action_trace)
//             (resource_balances)(voter_infos)(currency_balances)(last_irreversible_block) )


FC_REFLECT( graphene::zmq_plugin::currency_balance,
            (account_name)(balance)(deleted) )

FC_REFLECT( graphene::zmq_plugin::zmq_operation_object,
            (block_num)(block_time)(trx_id)(operation_trace)(currency_balances)(last_irreversible_block) )

FC_REFLECT( graphene::zmq_plugin::zmq_fork_block_object,
            (invalid_block_num) )

FC_REFLECT( graphene::zmq_plugin::zmq_accepted_block_object,
            (accepted_block_num)(accepted_block_timestamp)(accepted_block_witness)(accepted_witness_signature) )


FC_REFLECT( graphene::zmq_plugin::zmq_accounts_info_object,
            (currency_balances) )
