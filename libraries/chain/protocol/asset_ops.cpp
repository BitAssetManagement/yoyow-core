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
 *  They can contain a maximum of one '.'
 */
bool is_valid_symbol( const string& symbol )
{
    if( symbol.size() < GRAPHENE_MIN_ASSET_SYMBOL_LENGTH )
        return false;

    if( symbol.size() > GRAPHENE_MAX_ASSET_SYMBOL_LENGTH )
        return false;

    if( symbol.front() < 'A' || symbol.front() > 'Z' )
        return false;

    auto symb4 = symbol.substr( 0, 4 );
    if( symb4 == "YOYO" || symb4 == "YOY0" || symb4 == "Y0YO" || symb4 == "Y0Y0" )
       return false;

    bool dot_already_present = false;
    for( const auto c : symbol )
    {
        if( ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) )
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
   share_type core_fee_required = k.fee;
   if( memo )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(memo), k.price_per_kbyte );
   return core_fee_required;
}

share_type asset_create_operation::calculate_fee(const asset_create_operation::fee_parameters_type& param)const
{
   share_type core_fee_required = param.long_symbol;

   switch(symbol.size()) {
      case 3: core_fee_required = param.symbol3;
          break;
      case 4: core_fee_required = param.symbol4;
          break;
      default:
          break;
   }

   // common_options contains several lists and a string. Charge fees for its size
   core_fee_required += calculate_data_fee( common_options.data_size_for_fee(), param.price_per_kbyte );

   return core_fee_required;
}

void  asset_create_operation::validate()const
{
   validate_op_fee( fee, "asset create " );
   validate_account_uid( issuer, "asset create ");
   FC_ASSERT( is_valid_symbol(symbol) );
   common_options.validate();

   FC_ASSERT( precision <= 12, "precision should be no more than 12" );

   if( extensions.valid() )
   {
      const auto& ev = extensions->value;

      bool non_empty_extensions = ( ev.initial_supply.valid() );
      FC_ASSERT( non_empty_extensions, "extensions specified but is empty" );

      bool positive_initial_supply = ( *ev.initial_supply > 0 );
      FC_ASSERT( positive_initial_supply, "initial supply should be positive" );

      bool initial_supply_more_than_max = ( *ev.initial_supply > common_options.max_supply );
      FC_ASSERT( !initial_supply_more_than_max, "initial supply should not be more than max supply" );
   }
}

void asset_update_operation::validate()const
{
   validate_op_fee( fee, "asset update " );
   validate_account_uid( issuer, "asset update ");
   if( new_precision.valid() )
   {
      FC_ASSERT( *new_precision <= 12, "new precision should be no more than 12" );
   }
   new_options.validate();
}

share_type asset_update_operation::calculate_fee(const asset_update_operation::fee_parameters_type& param)const
{
   return share_type( param.fee ) + calculate_data_fee( new_options.data_size_for_fee(), param.price_per_kbyte );
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
   // TODO move to evaluator when enabling market
   FC_ASSERT( market_fee_percent == 0 );
   FC_ASSERT( max_market_fee == 0 );

   FC_ASSERT( max_supply > 0 );
   FC_ASSERT( max_supply <= GRAPHENE_MAX_SHARE_SUPPLY );
   FC_ASSERT( market_fee_percent <= GRAPHENE_100_PERCENT );
   FC_ASSERT( max_market_fee >= 0 && max_market_fee <= GRAPHENE_MAX_SHARE_SUPPLY );
   // There must be no high bits in permissions whose meaning is not known.
   FC_ASSERT( !(issuer_permissions & ~ASSET_ISSUER_PERMISSION_MASK) );
   // There must be no high bits in flags whose meaning is not known.
   FC_ASSERT( !(flags & ~ASSET_ISSUER_PERMISSION_MASK) );
   // The global_settle flag may never be set (this is a permission only)
   //FC_ASSERT( !(flags & global_settle) );
   // the witness_fed and committee_fed flags cannot be set simultaneously
   //FC_ASSERT( (flags & (witness_fed_asset | committee_fed_asset)) != (witness_fed_asset | committee_fed_asset) );

   // TODO move to evaluator when enabling account whitelisting feature with a hard fork
   FC_ASSERT( whitelist_authorities.empty() && blacklist_authorities.empty() );

   // TODO move to evaluator when enabling market whitelisting feature with a hard fork
   FC_ASSERT( whitelist_markets.empty() && blacklist_markets.empty() );

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
