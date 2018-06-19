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

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/csaf_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>

namespace graphene { namespace app {
   using namespace graphene::chain;

   struct full_account
   {
      account_object                   account;
      account_statistics_object        statistics;
      string                           registrar_name;
      string                           referrer_name;
      string                           lifetime_referrer_name;
      vector<proposal_object>          proposals;
      vector<csaf_lease_object>        csaf_leases_in;
      vector<csaf_lease_object>        csaf_leases_out;
      optional<voter_object>           voter;
      optional<witness_object>         witness;
      vector<account_uid_type>         witness_votes;
      optional<committee_member_object>    committee_member;
      vector<account_uid_type>         committee_member_votes;
      vector<asset_aid_type>           assets;
      vector<account_balance_object>   balances;
   };

} }

FC_REFLECT( graphene::app::full_account,
            (account)
            (statistics)
            //(registrar_name)
            //(referrer_name)
            //(lifetime_referrer_name)
            //(proposals)
            (csaf_leases_in)
            (csaf_leases_out)
            (voter)
            (witness)
            (witness_votes)
            (committee_member)
            (committee_member_votes)
            (assets)
            (balances)
          )
