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
#include <fc/utf8.hpp>
#include <graphene/chain/protocol/account.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/utilities/string_escape.hpp>

namespace graphene { namespace chain {

void validate_account_name( const string& name, const string& object_name = "" )
{
   const auto len = name.size();
   FC_ASSERT( len >= GRAPHENE_MIN_ACCOUNT_NAME_LENGTH, "${o}account name is too short", ("o", object_name) );
   FC_ASSERT( len <= GRAPHENE_MAX_ACCOUNT_NAME_LENGTH, "${o}account name is too long", ("o", object_name) );
   FC_ASSERT( fc::is_utf8( name ), "${o}account name should be in UTF8", ("o", object_name) );
   FC_ASSERT( !utilities::is_number( name ), "${o}account name should not be a number", ("o", object_name) );
}

void validate_new_authority( const authority& au, const string& object_name = "" )
{
   FC_ASSERT( au.num_auths() != 0, "${o}authority should contain something", ("o", object_name) );
   FC_ASSERT( au.address_auths.size() == 0, "cannot use address_auth in ${o}authority", ("o", object_name) );
   FC_ASSERT( au.account_auths.size() == 0, "account_auth deprecated, use account_uid_auth instead in ${o}authority", ("o", object_name) );
   string uid_check_obj_name = object_name + "authority ";
   for( const auto& a : au.account_uid_auths )
      validate_account_uid( a.first.uid, uid_check_obj_name );
   FC_ASSERT( !au.is_impossible(), "cannot use an imposible ${o}authority threshold", ("o", object_name) );
}

/**
 * Names must comply with the following grammar (RFC 1035):
 * <domain> ::= <subdomain> | " "
 * <subdomain> ::= <label> | <subdomain> "." <label>
 * <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
 * <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
 * <let-dig-hyp> ::= <let-dig> | "-"
 * <let-dig> ::= <letter> | <digit>
 *
 * Which is equivalent to the following:
 *
 * <domain> ::= <subdomain> | " "
 * <subdomain> ::= <label> ("." <label>)*
 * <label> ::= <letter> [ [ <let-dig-hyp>+ ] <let-dig> ]
 * <let-dig-hyp> ::= <let-dig> | "-"
 * <let-dig> ::= <letter> | <digit>
 *
 * I.e. a valid name consists of a dot-separated sequence
 * of one or more labels consisting of the following rules:
 *
 * - Each label is three characters or more
 * - Each label begins with a letter
 * - Each label ends with a letter or digit
 * - Each label contains only letters, digits or hyphens
 *
 * In addition we require the following:
 *
 * - All letters are lowercase
 * - Length is between (inclusive) GRAPHENE_MIN_ACCOUNT_NAME_LENGTH and GRAPHENE_MAX_ACCOUNT_NAME_LENGTH
 */
bool is_valid_name( const string& name )
{ try {
    const size_t len = name.size();

    /** this condition will prevent witnesses from including new names before this time, but
     * allow them after this time.   This check can be removed from the code after HARDFORK_385_TIME
     * has passed.
     */
    if( fc::time_point::now() < fc::time_point(HARDFORK_385_TIME) )
       FC_ASSERT( len >= 3 );

    if( len < GRAPHENE_MIN_ACCOUNT_NAME_LENGTH )
    {
          ilog( ".");
        return false;
    }

    if( len > GRAPHENE_MAX_ACCOUNT_NAME_LENGTH )
    {
          ilog( ".");
        return false;
    }

    size_t begin = 0;
    while( true )
    {
       size_t end = name.find_first_of( '.', begin );
       if( end == std::string::npos )
          end = len;
       if( (end - begin) < GRAPHENE_MIN_ACCOUNT_NAME_LENGTH )
       {
          idump( (name) (end)(len)(begin)(GRAPHENE_MAX_ACCOUNT_NAME_LENGTH) );
          return false;
       }
       switch( name[begin] )
       {
          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
          case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
          case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
          case 'y': case 'z':
             break;
          default:
          ilog( ".");
             return false;
       }
       switch( name[end-1] )
       {
          case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
          case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
          case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
          case 'y': case 'z':
          case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
          case '8': case '9':
             break;
          default:
          ilog( ".");
             return false;
       }
       for( size_t i=begin+1; i<end-1; i++ )
       {
          switch( name[i] )
          {
             case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
             case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
             case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
             case 'y': case 'z':
             case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
             case '8': case '9':
             case '-':
                break;
             default:
          ilog( ".");
                return false;
          }
       }
       if( end == len )
          break;
       begin = end+1;
    }
    return true;
} FC_CAPTURE_AND_RETHROW( (name) ) }

bool is_cheap_name( const string& n )
{
   bool v = false;
   for( auto c : n )
   {
      if( c >= '0' && c <= '9' ) return true;
      if( c == '.' || c == '-' || c == '/' ) return true;
      switch( c )
      {
         case 'a':
         case 'e':
         case 'i':
         case 'o':
         case 'u':
         case 'y':
            v = true;
      }
   }
   if( !v )
      return true;
   return false;
}

void account_options::validate() const
{
   validate_account_uid( voting_account, "voting_account " );
   //extensions.validate();
}

void account_reg_info::validate() const
{
   validate_account_uid( registrar, "registrar " );
   validate_account_uid( referrer, "referrer " );
   validate_percentage( registrar_percent, "registrar_percent" );
   validate_percentage( referrer_percent,  "referrer_percent"  );
   validate_percentage( registrar_percent + referrer_percent, "registrar_percent plus referrer_percent" );
   validate_percentage( buyout_percent, "buyout_percent" );
   // assets should be core asset
   validate_asset_id( allowance_per_article, "allowance_per_article" );
   validate_asset_id( max_share_per_article, "max_share_per_article" );
   validate_asset_id( max_share_total,       "max_share_total"       );
   // ==== checks below are not needed
   // allowance_per_article should be >= 0
   // max_share_per_article should be >= 0
   // max_share_total should be >= 0

   //extensions.validate();
}

share_type account_create_operation::calculate_fee( const fee_parameters_type& k )const
{
   auto core_fee_required = k.basic_fee;

   //if( !is_cheap_name(name) )
   //   core_fee_required = k.premium_fee;

   // Authorities and vote lists can be arbitrarily large, so charge a data fee for big ones
   auto data_size = fc::raw::pack_size(owner) + fc::raw::pack_size(active) + fc::raw::pack_size(secondary);
   auto data_fee =  calculate_data_fee( data_size, k.price_per_kbyte );
   core_fee_required += data_fee;

   return core_fee_required;
}


void account_create_operation::validate()const
{
   validate_op_fee( fee, "account creation " );
   validate_account_uid( uid, "new " );
   validate_account_name( name, "new " );
   validate_new_authority( owner, "new owner " );
   validate_new_authority( active, "new active " );
   validate_new_authority( secondary, "new secondary " );
   options.validate();
   reg_info.validate();
   if( extensions.valid() && extensions->value.owner_special_authority.valid() )
      validate_special_authority( *extensions->value.owner_special_authority );
   if( extensions.valid() && extensions->value.active_special_authority.valid() )
      validate_special_authority( *extensions->value.active_special_authority );
   if( extensions.valid() && extensions->value.buyback_options.valid() )
   {
      FC_ASSERT( !(extensions->value.owner_special_authority.valid()) );
      FC_ASSERT( !(extensions->value.active_special_authority.valid()) );
      FC_ASSERT( owner == authority::null_authority() );
      FC_ASSERT( active == authority::null_authority() );
      size_t n_markets = extensions->value.buyback_options->markets.size();
      FC_ASSERT( n_markets > 0 );
      for( const asset_id_type m : extensions->value.buyback_options->markets )
      {
         FC_ASSERT( m != extensions->value.buyback_options->asset_to_buy );
      }
   }
}

void account_manage_operation::validate()const
{
   validate_op_fee( fee, "account manage " );
   validate_account_uid( executor, "executor " );
   validate_account_uid( account, "target " );
   const auto& o = options.value;
   bool has_option = o.can_post.valid() || o.can_reply.valid() || o.can_rate.valid();
   FC_ASSERT( has_option, "Should update something" );
}


share_type account_update_operation::calculate_fee( const fee_parameters_type& k )const
{
   auto core_fee_required = k.fee;  
   if( new_options )
      core_fee_required += calculate_data_fee( fc::raw::pack_size(*this), k.price_per_kbyte );
   return core_fee_required;
}

void account_update_operation::validate()const
{
   FC_ASSERT( account != GRAPHENE_TEMP_ACCOUNT );
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( account != account_id_type() );

   bool has_action = (
         owner.valid()
      || active.valid()
      || new_options.valid()
      || extensions.value.owner_special_authority.valid()
      || extensions.value.active_special_authority.valid()
      );

   FC_ASSERT( has_action );

   if( owner )
   {
      FC_ASSERT( owner->num_auths() != 0 );
      FC_ASSERT( owner->address_auths.size() == 0 );
      FC_ASSERT( !owner->is_impossible(), "cannot update an account with an imposible owner authority threshold" );
   }
   if( active )
   {
      FC_ASSERT( active->num_auths() != 0 );
      FC_ASSERT( active->address_auths.size() == 0 );
      FC_ASSERT( !active->is_impossible(), "cannot update an account with an imposible active authority threshold" );
   }

   if( new_options )
      new_options->validate();
   if( extensions.value.owner_special_authority.valid() )
      validate_special_authority( *extensions.value.owner_special_authority );
   if( extensions.value.active_special_authority.valid() )
      validate_special_authority( *extensions.value.active_special_authority );
}

share_type account_upgrade_operation::calculate_fee(const fee_parameters_type& k) const
{
   if( upgrade_to_lifetime_member )
      return k.membership_lifetime_fee;
   return k.membership_annual_fee;
}


void account_upgrade_operation::validate() const
{
   FC_ASSERT( fee.amount >= 0 );
}

void account_transfer_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}


} } // graphene::chain
