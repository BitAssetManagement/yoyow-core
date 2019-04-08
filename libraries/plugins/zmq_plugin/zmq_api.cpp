
#include <fc/filesystem.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>
#include <fc/smart_ref_impl.hpp>

#include <graphene/app/application.hpp>

#include <graphene/chain/block_database.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/witness_object.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <graphene/zmq_plugin/zmq_api.hpp>
#include <graphene/zmq_plugin/zmq_plugin.hpp>

namespace graphene { namespace zmq_plugin {

zmq_api::zmq_api( graphene::app::application& app ):_app(app)
{
   
}

std::shared_ptr<zmq_plugin> zmq_api::get_plugin(){
    return _app.get_plugin<zmq_plugin>("zmq_plugin");
}

std::string zmq_api::get_accounts_balances(const std::vector<asset_aid_type>& assets_id){
    auto zp = get_plugin();
    return zp->get_accounts_balances(assets_id);
}

std::string zmq_api::get_account_balances(const account_uid_type owner, const std::vector<asset_aid_type>& assets_id){
    auto zp = get_plugin();
    return zp->get_account_balances(owner, assets_id);
}


} } // graphene::zmq_plugin
