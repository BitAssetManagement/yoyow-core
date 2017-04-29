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
#include <graphene/chain/content_evaluator.hpp>
#include <graphene/chain/content_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

void_result post_evaluator::do_evaluate( const post_operation& op )
{ try {

   const database& d = db();
/*

   from_account    = &d.get_account_by_uid( op.from );
   to_account      = &d.get_account_by_uid( op.to );

   const asset_object&   transfer_asset_object      = asset_id_type(op.amount.asset_id)(d);

   try {

      GRAPHENE_ASSERT(
         is_authorized_asset( d, *from_account, transfer_asset_object ),
         transfer_from_account_not_whitelisted,
         "'from' account ${from} is not whitelisted for asset ${asset}",
         ("from",op.from)
         ("asset",op.amount.asset_id)
         );
      GRAPHENE_ASSERT(
         is_authorized_asset( d, *to_account, transfer_asset_object ),
         transfer_to_account_not_whitelisted,
         "'to' account ${to} is not whitelisted for asset ${asset}",
         ("to",op.to)
         ("asset",op.amount.asset_id)
         );

      if( transfer_asset_object.is_transfer_restricted() )
      {
         GRAPHENE_ASSERT(
            from_account->id == transfer_asset_object.issuer || to_account->id == transfer_asset_object.issuer,
            transfer_restricted_transfer_asset,
            "Asset {asset} has transfer_restricted flag enabled",
            ("asset", op.amount.asset_id)
          );
      }

      const auto& from_balance = d.get_balance( *from_account, transfer_asset_object );
      bool sufficient_balance = from_balance.amount >= op.amount.amount;
      FC_ASSERT( sufficient_balance,
                 "Insufficient Balance: ${balance}, unable to transfer '${a}' from account '${f}' to '${t}'",
                 ("f",from_account->uid)("t",to_account->uid)("a",d.to_pretty_string(op.amount))("balance",d.to_pretty_string(from_balance)) );

      return void_result();
   } FC_RETHROW_EXCEPTIONS( error, "Unable to transfer ${a} from ${f} to ${t}", ("a",d.to_pretty_string(op.amount))("f",from_account->uid)("t",to_account->uid) );
*/

   return void_result();

}  FC_CAPTURE_AND_RETHROW( (op) ) }

void_result post_evaluator::do_apply( const post_operation& o )
{ try {
   //db().adjust_balance( *from_account, -o.amount );
   //db().adjust_balance( *to_account, o.amount );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
