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

#include <memory>
#include <string>

#include <fc/api.hpp>
#include <fc/variant_object.hpp>

#include <graphene/zmq_plugin/zmq_plugin.hpp>

namespace graphene { namespace app {
class application;
} }

namespace graphene { namespace zmq_plugin {

class zmq_api
{
   public:
      zmq_api( graphene::app::application& app ); 

      std::shared_ptr<zmq_plugin> get_plugin();
      std::string get_accounts_balances(const std::vector<asset_aid_type>& assets_id);
      std::string get_account_balances(const account_uid_type owner, const std::vector<asset_aid_type>& assets_id);

    private:
       graphene::app::application& _app; 
};

} }

FC_API(graphene::zmq_plugin::zmq_api,
       (get_accounts_balances)
       (get_account_balances)
     )
