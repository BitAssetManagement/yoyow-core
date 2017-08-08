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
#include <graphene/chain/witness_evaluator.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/vote.hpp>

namespace graphene { namespace chain {

void_result witness_create_evaluator::do_evaluate( const witness_create_operation& op )
{ try {
   database& d = db();
   //FC_ASSERT(db().get(op.witness_account).is_lifetime_member());
   account_stats = &d.get_account_statistics_by_uid( op.witness_account );

   const auto& global_params = d.get_global_properties().parameters;
   // TODO review
   if( d.head_block_num() > 0 )
      FC_ASSERT( op.pledge.amount >= global_params.min_witness_pledge,
                 "Insufficient pledge: provided ${p}, need ${r}",
                 ("p",d.to_pretty_string(op.pledge))
                 ("r",d.to_pretty_core_string(global_params.min_witness_pledge)) );

   auto available_balance = account_stats->core_balance - account_stats->core_leased_out; // releasing pledge can be reused.
   FC_ASSERT( available_balance >= op.pledge.amount,
              "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
              ("a",op.witness_account)
              ("b",d.to_pretty_core_string(available_balance))
              ("r",d.to_pretty_string(op.pledge)) );

   const witness_object* maybe_found = d.find_witness_by_uid( op.witness_account );
   FC_ASSERT( maybe_found == nullptr, "This account is already a witness" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type witness_create_evaluator::do_apply( const witness_create_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   const auto& new_witness_object = d.create<witness_object>( [&]( witness_object& wit ){
      wit.witness_account     = op.witness_account;
      wit.sequence            = account_stats->last_witness_sequence + 1;
      //wit.is_valid            = true; // default
      wit.signing_key         = op.block_signing_key;
      wit.pledge              = op.pledge.amount.value;
      wit.pledge_last_update  = d.head_block_time();
      //wit.vote_id           = vote_id;

      wit.average_pledge_last_update       = d.head_block_time();
      if( wit.pledge > 0 )
         wit.average_pledge_next_update_block = d.head_block_num() + global_params.witness_avg_pledge_update_interval;
      else // init witnesses
         wit.average_pledge_next_update_block = -1;

      wit.url                 = op.url;

   });

   d.modify( *account_stats, [&](account_statistics_object& s) {
      s.last_witness_sequence += 1;
      if( s.releasing_witness_pledge > op.pledge.amount )
         s.releasing_witness_pledge -= op.pledge.amount;
      else
      {
         s.total_witness_pledge = op.pledge.amount;
         if( s.releasing_witness_pledge > 0 )
         {
            s.releasing_witness_pledge = 0;
            s.witness_pledge_release_block_number = -1;
         }
      }
   });

   return new_witness_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_evaluate( const witness_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.witness_account );
   witness_obj   = &d.get_witness_by_uid( op.witness_account );

   const auto& global_params = d.get_global_properties().parameters;
   if( op.new_signing_key.valid() )
      FC_ASSERT( *op.new_signing_key != witness_obj->signing_key, "new_signing_key specified but did not change" );
   if( op.new_pledge.valid() )
   {
      if( op.new_pledge->amount > 0 ) // change pledge
      {
         FC_ASSERT( op.new_pledge->amount >= global_params.min_witness_pledge,
                    "Insufficient pledge: provided ${p}, need ${r}",
                    ("p",d.to_pretty_string(*op.new_pledge))
                    ("r",d.to_pretty_core_string(global_params.min_witness_pledge)) );

         FC_ASSERT( op.new_pledge->amount != witness_obj->pledge, "new_pledge specified but did not change" );

         auto available_balance = account_stats->core_balance - account_stats->core_leased_out; // releasing pledge can be reused.
         FC_ASSERT( available_balance >= op.new_pledge->amount,
                    "Insufficient Balance: account ${a}'s available balance of ${b} is less than required ${r}",
                    ("a",op.witness_account)
                    ("b",d.to_pretty_core_string(available_balance))
                    ("r",d.to_pretty_string(*op.new_pledge)) );
      }
      else if( op.new_pledge->amount == 0 ) // resign
      {
         const auto& active_witnesses = d.get_global_properties().active_witnesses;
         FC_ASSERT( active_witnesses.find( op.witness_account ) == active_witnesses.end(),
                    "Active witness can not resign" );
      }
   }
   if( op.new_url.valid() )
      FC_ASSERT( *op.new_url != witness_obj->url, "new_url specified but did not change" );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_update_evaluator::do_apply( const witness_update_operation& op )
{ try {
   database& d = db();

   const auto& global_params = d.get_global_properties().parameters;

   if( !op.new_pledge.valid() ) // change url or key
   {
      d.modify( *witness_obj, [&]( witness_object& wit ) {
         if( op.new_signing_key.valid() )
            wit.signing_key = *op.new_signing_key;
         if( op.new_url.valid() )
            wit.url = *op.new_url;
      });
   }
   else if( op.new_pledge->amount == 0 ) // resign
   {
      d.modify( *account_stats, [&](account_statistics_object& s) {
         s.releasing_witness_pledge = s.total_witness_pledge;
         s.witness_pledge_release_block_number = d.head_block_num() + global_params.witness_pledge_release_delay;
      });
      d.modify( *witness_obj, [&]( witness_object& wit ) {
         wit.is_valid = false; // will be processed later
         wit.average_pledge_next_update_block = -1;
         wit.by_pledge_scheduled_time = fc::uint128_t::max_value();
         wit.by_vote_scheduled_time = fc::uint128_t::max_value();
      });
   }
   else // change pledge
   {
      // update account stats
      share_type delta = op.new_pledge->amount - witness_obj->pledge;
      if( delta > 0 ) // more pledge
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            if( s.releasing_witness_pledge > delta )
               s.releasing_witness_pledge -= delta;
            else
            {
               s.total_witness_pledge = op.new_pledge->amount;
               if( s.releasing_witness_pledge > 0 )
               {
                  s.releasing_witness_pledge = 0;
                  s.witness_pledge_release_block_number = -1;
               }
            }
         });
      }
      else // less pledge
      {
         d.modify( *account_stats, [&](account_statistics_object& s) {
            s.releasing_witness_pledge -= delta;
            s.witness_pledge_release_block_number = d.head_block_num() + global_params.witness_pledge_release_delay;
         });
      }

      // update position
      d.update_witness_avg_pledge( *witness_obj );

      // update witness data
      d.modify( *witness_obj, [&]( witness_object& wit ) {
         if( op.new_signing_key.valid() )
            wit.signing_key = *op.new_signing_key;

         wit.pledge              = op.new_pledge->amount.value;
         wit.pledge_last_update  = d.head_block_time();

         if( op.new_url.valid() )
            wit.url = *op.new_url;
      });

      // update schedule
      d.update_witness_avg_pledge( *witness_obj );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_update_evaluator::do_evaluate( const witness_vote_update_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.voter );

   const auto& global_params = d.get_global_properties().parameters;
   FC_ASSERT( account_stats->core_balance >= global_params.min_governance_voting_balance,
              "Need more balance to be able to vote: have ${b}, need ${r}",
              ("b",d.to_pretty_core_string(account_stats->core_balance))
              ("r",d.to_pretty_core_string(global_params.min_governance_voting_balance)) );

   const auto max_witnesses = global_params.max_witnesses_voted_per_account;
   FC_ASSERT( op.witnesses_to_add.size() <= max_witnesses,
              "Trying to vote for ${n} witnesses, more than allowed maximum: ${m}",
              ("n",op.witnesses_to_add.size())("m",max_witnesses) );

   for( const auto uid : op.witnesses_to_remove )
   {
      const witness_object& wit = d.get_witness_by_uid( uid );
      witnesses_to_remove.push_back( &wit );
   }
   for( const auto uid : op.witnesses_to_add )
   {
      const witness_object& wit = d.get_witness_by_uid( uid );
      witnesses_to_add.push_back( &wit );
   }

   if( account_stats->is_voter ) // maybe valid voter
   {
      voter_obj = d.find_voter( op.voter, account_stats->last_voter_sequence );
      FC_ASSERT( voter_obj != nullptr, "voter should exist" );

      // check if the voter is still valid
      bool still_valid = d.check_voter_valid( *voter_obj, true );
      if( !still_valid )
      {
         invalid_voter_obj = voter_obj;
         voter_obj = nullptr;
      }
   }
   // else do nothing

   if( voter_obj == nullptr ) // not voting
      FC_ASSERT( op.witnesses_to_remove.empty(), "Not voting for any witness, or votes were no longer valid, can not remove" );
   else if( voter_obj->proxy_uid != GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID ) // voting with a proxy
   {
      // check if the proxy is still valid
      const voter_object* current_proxy_voter_obj = d.find_voter( voter_obj->proxy_uid, voter_obj->proxy_sequence );
      FC_ASSERT( current_proxy_voter_obj != nullptr, "proxy voter should exist" );
      bool current_proxy_still_valid = d.check_voter_valid( *current_proxy_voter_obj, true );
      if( current_proxy_still_valid ) // still valid
         FC_ASSERT( op.witnesses_to_remove.empty() && op.witnesses_to_add.empty(),
                    "Now voting with a proxy, can not add or remove witness" );
      else // no longer valid
      {
         invalid_current_proxy_voter_obj = current_proxy_voter_obj;
         FC_ASSERT( op.witnesses_to_remove.empty(),
                    "Was voting with a proxy but it is now invalid, so not voting for any witness, can not remove" );
      }
   }
   else // voting by self
   {
      // check for voted witnesses whom have became invalid
      uint16_t witnesses_voted = voter_obj->number_of_witnesses_voted;
      const auto& idx = d.get_index_type<witness_vote_index>().indices().get<by_voter_seq>();
      auto itr = idx.lower_bound( std::make_tuple( op.voter, voter_obj->sequence ) );
      while( itr != idx.end() && itr->voter_uid == op.voter && itr->voter_sequence == voter_obj->sequence )
      {
         const witness_object* witness = d.find_witness_by_uid( itr->witness_uid );
         if( witness == nullptr || witness->sequence != itr->witness_sequence )
         {
            invalid_witness_votes_to_remove.push_back( &(*itr) );
            --witnesses_voted;
         }
         ++itr;
      }

      FC_ASSERT( op.witnesses_to_remove.size() <= witnesses_voted,
                 "Trying to remove ${n} witnesses, more than voted: ${m}",
                 ("n",op.witnesses_to_remove.size())("m",witnesses_voted) );
      uint16_t new_total = witnesses_voted - op.witnesses_to_remove.size() + op.witnesses_to_add.size();
      FC_ASSERT( new_total <= max_witnesses,
                 "Trying to vote for ${n} witnesses, more than allowed maximum: ${m}",
                 ("n",new_total)("m",max_witnesses) );

      for( const auto& wit : witnesses_to_remove )
      {
         const witness_vote_object* wit_vote = d.find_witness_vote( op.voter, voter_obj->sequence,
                                                                    wit->witness_account, wit->sequence );
         FC_ASSERT( wit_vote != nullptr, "Not voting for witness ${w}, can not remove", ("w",wit->witness_account) );
         witness_votes_to_remove.push_back( wit_vote );
      }
      for( const auto& wit : witnesses_to_add )
      {
         const witness_vote_object* wit_vote = d.find_witness_vote( op.voter, voter_obj->sequence,
                                                                    wit->witness_account, wit->sequence );
         FC_ASSERT( wit_vote == nullptr, "Already voting for witness ${w}, can not add", ("w",wit->witness_account) );
      }
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_vote_update_evaluator::do_apply( const witness_vote_update_operation& op )
{ try {
   database& d = db();
   const auto head_block_time = d.head_block_time();
   const auto head_block_num  = d.head_block_num();
   const auto& global_params = d.get_global_properties().parameters;
   const auto max_level = global_params.max_governance_voting_proxy_level;

   if( invalid_current_proxy_voter_obj != nullptr )
      d.invalidate_voter( *invalid_current_proxy_voter_obj );

   if( invalid_voter_obj != nullptr )
      d.invalidate_voter( *invalid_voter_obj );

   share_type total_votes = 0;
   if( voter_obj != nullptr ) // voter already exists
   {
      // clear proxy votes
      if( invalid_current_proxy_voter_obj != nullptr )
      {
         d.clear_voter_proxy_votes( *voter_obj );
         // update proxy
         d.modify( *invalid_current_proxy_voter_obj, [&](voter_object& v) {
            v.proxied_voters -= 1;
         });
      }

      // remove invalid witness votes
      for( const auto& wit_vote : invalid_witness_votes_to_remove )
         d.remove( *wit_vote );

      // remove requested witness votes
      total_votes = voter_obj->total_votes();
      const size_t total_remove = witnesses_to_remove.size();
      for( uint16_t i = 0; i < total_remove; ++i )
      {
         d.adjust_witness_votes( *witnesses_to_remove[i], -total_votes );
         d.remove( *witness_votes_to_remove[i] );
      }

      d.modify( *voter_obj, [&](voter_object& v) {
         // update voter proxy to self
         if( invalid_current_proxy_voter_obj != nullptr )
         {
            v.proxy_uid      = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
            v.proxy_sequence = 0;
         }
         v.proxy_last_vote_block[0]  = head_block_num;
         v.effective_last_vote_block = head_block_num;
         v.number_of_witnesses_voted = v.number_of_witnesses_voted
                                     - invalid_witness_votes_to_remove.size()
                                     - witnesses_to_remove.size()
                                     + witnesses_to_add.size();
      });
   }
   else // need to create a new voter object for this account
   {
      d.modify( *account_stats, [&](account_statistics_object& s) {
         s.is_voter = true;
         s.last_voter_sequence += 1;
      });

      voter_obj = &d.create<voter_object>( [&]( voter_object& v ){
         v.uid               = op.voter;
         v.sequence          = account_stats->last_voter_sequence;
         //v.is_valid          = true; // default
         v.votes             = account_stats->core_balance.value;
         v.votes_last_update = head_block_time;

         //v.effective_votes                 = 0; // default
         v.effective_votes_last_update       = head_block_time;
         v.effective_votes_next_update_block = head_block_num + global_params.governance_votes_update_interval;

         v.proxy_uid         = GRAPHENE_PROXY_TO_SELF_ACCOUNT_UID;
         // v.proxy_sequence = 0; // default

         //v.proxied_voters    = 0; // default
         v.proxied_votes.resize( max_level, 0 ); // [ level1, level2, ... ]
         v.proxy_last_vote_block.resize( max_level+1, 0 ); // [ self, proxy, proxy->proxy, ... ]
         v.proxy_last_vote_block[0] = head_block_num;

         v.effective_last_vote_block         = head_block_num;

         v.number_of_witnesses_voted         = witnesses_to_add.size();
      });
   }

   // add requested witness votes
   const size_t total_add = witnesses_to_add.size();
   for( uint16_t i = 0; i < total_add; ++i )
   {
      d.create<witness_vote_object>( [&]( witness_vote_object& o ){
         o.voter_uid        = op.voter;
         o.voter_sequence   = voter_obj->sequence;
         o.witness_uid      = witnesses_to_add[i]->witness_account;
         o.witness_sequence = witnesses_to_add[i]->sequence;
      });
      if( total_votes > 0 )
         d.adjust_witness_votes( *witnesses_to_add[i], total_votes );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_collect_pay_evaluator::do_evaluate( const witness_collect_pay_operation& op )
{ try {
   database& d = db();
   account_stats = &d.get_account_statistics_by_uid( op.witness_account );

   FC_ASSERT( account_stats->uncollected_witness_pay >= op.pay.amount,
              "Can not collect so much: have ${b}, requested ${r}",
              ("b",d.to_pretty_core_string(account_stats->uncollected_witness_pay))
              ("r",d.to_pretty_string(op.pay)) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result witness_collect_pay_evaluator::do_apply( const witness_collect_pay_operation& op )
{ try {
   database& d = db();

   d.adjust_balance( op.witness_account, op.pay );
   d.modify( *account_stats, [&](account_statistics_object& s) {
      s.uncollected_witness_pay -= op.pay.amount;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain
