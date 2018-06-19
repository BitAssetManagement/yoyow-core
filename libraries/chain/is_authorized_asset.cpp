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

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

namespace graphene { namespace chain {

namespace detail {

bool _is_authorized_asset(
   const database& d,
   const account_object& acct,
   const asset_object& asset_obj)
{
   if( acct.allowed_assets.valid() )
   {
      if( acct.allowed_assets->find( asset_obj.asset_id ) == acct.allowed_assets->end() )
         return false;
      // must still pass other checks even if it is in allowed_assets
   }

   if( !( asset_obj.enabled_whitelist() ) ) // pass if not enabled whitelisting
      return true;

   for( const auto id : acct.blacklisting_accounts )
   {
      if( asset_obj.options.blacklist_authorities.find(id) != asset_obj.options.blacklist_authorities.end() )
         return false;
   }

   if( asset_obj.options.whitelist_authorities.size() == 0 )
      return true;

   for( const auto id : acct.whitelisting_accounts )
   {
      if( asset_obj.options.whitelist_authorities.find(id) != asset_obj.options.whitelist_authorities.end() )
         return true;
   }

   return false;
}

void _validate_authorized_asset( const database& d,
                                 const account_object& acct,
                                 const asset_object& asset_obj,
                                 const string& account_desc_prefix )
{
   if( acct.allowed_assets.valid() )
   {
      bool is_allowed_asset = ( acct.allowed_assets->find( asset_obj.asset_id ) != acct.allowed_assets->end() );
      FC_ASSERT( is_allowed_asset,
                 "Asset '${asset}' is not allowed by ${prefix}account ${acc}",
                 ("asset", asset_obj.symbol)
                 ("prefix", account_desc_prefix)
                 ("acc", acct.uid) );
   }

   if( !( asset_obj.enabled_whitelist() ) ) // pass if not enabled whitelisting
      return;

   for( const auto id : acct.blacklisting_accounts )
   {
      bool is_blacklisted = ( asset_obj.options.blacklist_authorities.find(id) != asset_obj.options.blacklist_authorities.end() );
      FC_ASSERT( !is_blacklisted,
                 "${prefix}account ${acc} is blacklisted for asset '${asset}'",
                 ("prefix", account_desc_prefix)
                 ("acc", acct.uid)
                 ("asset", asset_obj.symbol) );
   }

   if( asset_obj.options.whitelist_authorities.size() == 0 ) // pass if no whitelist authority configured
      return;

   for( const auto id : acct.whitelisting_accounts )
   {
      // pass if whitelisted by any one of whitelist authorities
      if( asset_obj.options.whitelist_authorities.find(id) != asset_obj.options.whitelist_authorities.end() )
         return;
   }

   bool is_whitelisted = false;
   FC_ASSERT( is_whitelisted,
              "${prefix}account ${acc} is not whitelisted for asset '${asset}'",
              ("prefix", account_desc_prefix)
              ("acc", acct.uid)
              ("asset", asset_obj.symbol) );
}

} // detail

} } // graphene::chain
