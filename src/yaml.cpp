#include "qlib/yaml.h"
#include "qlib/logger.h"
// #include "qlib/psdk.h"

// namespace qlib {

// namespace yaml {

// template <>
// decltype(psdk::init_parameter{}.usb.bulk1) loader::get<decltype(psdk::init_parameter{}.usb.bulk1)>(
//     node const& node) {
//     decltype(psdk::init_parameter{}.usb.bulk1) result;

//     get(&result.interface_num, node, "interface_num");
//     get(&result.endpoint_in, node, "endpoint_in");
//     get(&result.endpoint_out, node, "endpoint_out");
//     get(&result.ep_in, node, "ep_in");
//     get(&result.ep_out, node, "ep_out");

//     return result;
// }

// template <>
// psdk::init_parameter loader::get<psdk::init_parameter>(node const& node) {
//     psdk::init_parameter result;

//     get(&result.log_prefix, node, "log_prefix");
//     get(&result.connect_type, node, "connect_type");
//     get(&result.app_name, node, "app_name");
//     get(&result.app_id, node, "app_id");
//     get(&result.app_key, node, "app_key");
//     get(&result.app_license, node, "app_license");
//     get(&result.developer_account, node, "developer_account");
//     get(&result.baud_rate, node, "baud_rate");
//     get(&result.uart1, node, "uart1");
//     get(&result.uart2, node, "uart2");

//     {
//         auto& network_node = node["network"];
//         get(&result.network.name, network_node, "name");
//         get(&result.network.vid, network_node, "vid");
//         get(&result.network.pid, network_node, "pid");
//     }
//     {
//         auto& usb_node = node["usb"];
//         get(&result.usb.vid, usb_node, "vid");
//         get(&result.usb.pid, usb_node, "pid");
//         result.usb.bulk1 = get<decltype(result.usb.bulk1)>(usb_node, "bulk1");
//         result.usb.bulk2 = get<decltype(result.usb.bulk2)>(usb_node, "bulk2");
//     }

//     return result;
// }
// };  // namespace yaml

// };  // namespace qlib
