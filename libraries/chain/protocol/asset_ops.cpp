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
#include <graphene/chain/protocol/asset_ops.hpp>

namespace graphene { namespace chain {

/**
 *  Valid symbols can contain [A-Z0-9], and '.'
 *  They must start with [A, Z]
 *  They must end with [A, Z]
 *  They can contain a maximum of one '.'
 */
bool is_valid_symbol( const string& symbol )
{
    if( symbol.size() < GRAPHENE_MIN_ASSET_SYMBOL_LENGTH )
        return false;

    if( symbol.substr(0,3) == "BIT" ) 
       return false;

    if( symbol.size() > GRAPHENE_MAX_ASSET_SYMBOL_LENGTH )
        return false;

    if( !isalpha( symbol.front() ) )
        return false;

    if( !isalpha( symbol.back() ) )
        return false;

    bool dot_already_present = false;
    for( const auto c : symbol )
    {
        if( (isalpha( c ) && isupper( c )) || isdigit(c) )
            continue;

        if( c == '.' )
        {
            if( dot_already_present )
                return false;

            dot_already_present = true;
            continue;
        }

        return false;
    }

    return true;
}

share_type asset_issue_operation::calculate_fee(const fee_parameters_type& k)const
{
   return k.fee + calculate_data_fee( fc::raw::pack_size(memo), k.price_per_kbyte );
}

share_type asset_create_operation::calculate_fee(const asset_create_operation::fee_parameters_type& param)const
{
   auto core_fee_required = param.long_symbol; 

   switch(symbol.size()) {
      case 3: core_fee_required = param.symbol3;
          break;
      case 4: core_fee_required = param.symbol4;
          break;
      default:
          break;
   }

   // common_options contains several lists and a string. Charge fees for its size
   core_fee_required += calculate_data_fee( fc::raw::pack_size(*this), param.price_per_kbyte );

   return core_fee_required;
}

void  asset_create_operation::validate()const
{
   validate_op_fee( fee, "asset create " );
   validate_account_uid( issuer, "asset create ");
   FC_ASSERT( is_valid_symbol(symbol) );
   common_options.validate();

   FC_ASSERT(precision <= 12);
}

void asset_update_operation::validate()const
{
   validate_op_fee( fee, "asset update " );
   validate_account_uid( issuer, "asset update ");
   if( new_issuer.valid() )
   {
      validate_account_uid( *new_issuer, "asset update ");
      FC_ASSERT(issuer != *new_issuer);
   }
   new_options.validate();
}

share_type asset_update_operation::calculate_fee(const asset_update_operation::fee_parameters_type& k)const
{
   return k.fee + calculate_data_fee( fc::raw::pack_size(*this), k.price_per_kbyte );
}

void asset_reserve_operation::validate()const
{
   validate_op_fee( fee, "asset reserve " );
   validate_account_uid( payer, "asset reserve ");
   FC_ASSERT( amount_to_reserve.amount.value <= GRAPHENE_MAX_SHARE_SUPPLY );
   FC_ASSERT( amount_to_reserve.amount.value > 0 );
}

void asset_issue_operation::validate()const
{
   validate_op_fee( fee, "asset issue " );
   validate_account_uid( issuer, "asset issue ");
   validate_account_uid( issue_to_account, "asset issue ");
   FC_ASSERT( asset_to_issue.amount.value <= GRAPHENE_MAX_SHARE_SUPPLY );
   FC_ASSERT( asset_to_issue.amount.value > 0 );
   FC_ASSERT( asset_to_issue.asset_id != GRAPHENE_CORE_ASSET_AID );
}

void asset_options::validate()const
{
   FC_ASSERT( max_supply > 0 );
   FC_ASSERT( max_supply <= GRAPHENE_MAX_SHARE_SUPPLY );
   FC_ASSERT( market_fee_percent <= GRAPHENE_100_PERCENT );
   FC_ASSERT( max_market_fee >= 0 && max_market_fee <= GRAPHENE_MAX_SHARE_SUPPLY );
   // There must be no high bits in permissions whose meaning is not known.
   FC_ASSERT( !(issuer_permissions & ~UIA_ASSET_ISSUER_PERMISSION_MASK) );
   // The global_settle flag may never be set (this is a permission only)
   //FC_ASSERT( !(flags & global_settle) );
   // the witness_fed and committee_fed flags cannot be set simultaneously
   //FC_ASSERT( (flags & (witness_fed_asset | committee_fed_asset)) != (witness_fed_asset | committee_fed_asset) );

   if(!whitelist_authorities.empty() || !blacklist_authorities.empty())
      FC_ASSERT( flags & white_list );
   for( auto item : whitelist_markets )
   {
      FC_ASSERT( blacklist_markets.find(item) == blacklist_markets.end() );
   }
   for( auto item : blacklist_markets )
   {
      FC_ASSERT( whitelist_markets.find(item) == whitelist_markets.end() );
   }
}

void asset_claim_fees_operation::validate()const {
   validate_op_fee( fee, "asset claim fees " );
   validate_account_uid( issuer, "asset claim fees ");
   FC_ASSERT( amount_to_claim.amount > 0 );
}

} } // namespace graphene::chain
