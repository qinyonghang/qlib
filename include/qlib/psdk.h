#pragma once

#define PSDK_IMPLEMENTATION

#include <functional>
#include <vector>

#include "exception.h"
#include "object.h"
#include "singleton.h"

namespace qlib {

namespace psdk {

struct init_parameter {
    string log_prefix{"logs/psdk"};

    uint32_t connect_type;
    string app_name;
    string app_id;
    string app_key;
    string app_license;
    string developer_account;
    string baud_rate;

    // uart
    std::string uart1;
    std::string uart2;

    // network
    struct {
        string name;
        uint32_t vid;
        uint32_t pid;

        auto to_string() const { return fmt::format("[{},{},{}]", name, vid, pid); }
    } network;

    // usb
    struct {
        uint32_t vid;
        uint32_t pid;
        struct {
            string ep_in;
            string ep_out;
            uint16_t interface_num;
            uint16_t endpoint_in;
            uint16_t endpoint_out;
            auto to_string() const {
                return fmt::format("[{},{},{},{},{}]", ep_in, ep_out, interface_num, endpoint_in,
                                   endpoint_out);
            }
        } bulk1, bulk2;

        auto to_string() const { return fmt::format("[{},{},{},{}]", vid, pid, bulk1, bulk2); }
    } usb;

    auto to_string() const {
        return fmt::format("[{},{},{},{},{},{},{},{},{},{},{},{}]", log_prefix, connect_type,
                           app_name, app_id, app_key, app_license, developer_account, baud_rate,
                           uart1, uart2, network, usb);
    }
};

qlib::object<void>::ptr make(init_parameter const&);

class flight_control final : public qlib::object<flight_control> {
public:
    using base = qlib::object<flight_control>;
    using self = flight_control;
    using ptr = std::shared_ptr<self>;

    enum class action : uint32_t {
        no_action,
        take_off,
        land,
        waypoints,
        go_home = 1,
        auto_land = 2
    };

    enum result : int32_t {
        ok = 0,
        unknown_error = -1,
        timeout = -2,
        already_in_air = -3,
        already_landed = -4,
        no_satellite = -5
    };

    struct init_parameter {
        bool simulator{true};
        uint32_t number_of_valid_satellites{10u};
        float32_t global_flight_altitude{20};   // m
        float32_t global_flight_speed{10};      // m/s
        float32_t global_flight_speed_max{15};  // m/s
        uint32_t retry_max{30u};
    };

    struct parameter {
        using self = parameter;
        using ptr = std::shared_ptr<self>;
    };

    struct waypoints_parameter : public parameter {
        using self = waypoints_parameter;
        using ptr = std::shared_ptr<self>;

        std::vector<position> points;
        action finish_action{action::no_action};
        uint32_t repeat_times{0u};

        string to_string() const {
            std::string out;
            for (auto& point : points) {
                out += fmt::format("[{},{},{}]", point.longitude, point.latitude,
                                   point.relative_height);
            }
            return fmt::format("[[{}],{},{}]", out, static_cast<uint32_t>(finish_action),
                               repeat_times);
        }
    };

    flight_control(init_parameter const& parameter) {
        int32_t result{init(parameter)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(init_parameter const& parameter);

    int32_t set_action(action action,
                       parameter::ptr const& parameter_ptr = nullptr,
                       std::function<void(int32_t)> const& callback = nullptr);

    auto take_off(std::function<void(int32_t)> const& callback = nullptr) {
        return set_action(action::take_off, nullptr, callback);
    }
    auto land(std::function<void(int32_t)> const& callback = nullptr) {
        return set_action(action::land, nullptr, callback);
    }

    int32_t waypoints(waypoints_parameter::ptr const& parameter_ptr,
                      std::function<void(int32_t)> const& callback = nullptr) {
        return set_action(action::waypoints, parameter_ptr, callback);
    }

protected:
    qlib::object<void>::ptr impl_ptr;

    friend class qlib::ref_singleton<self>;
};

class camera final : public object<camera> {
public:
    using base = object<camera>;
    using self = camera;
    using ptr = std::shared_ptr<self>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return ref_singleton<self>::make(std::forward<Args>(args)...);
    }

    struct init_parameter : public base::parameter {};
    camera(init_parameter const& parameter) {
        int32_t result{init(parameter)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(init_parameter const& parameter);

    enum class index : uint32_t {
        fpv,
        h30t,
    };

    using frame = std::vector<uint8_t>;
    int32_t subscribe(index index, std::function<void(frame&&)> const& callback);
    int32_t unsubscribe(index index);

    struct parameter : public base::parameter {
        enum class stream : uint32_t { h264 } stream_type;
        uint32_t width;
        uint32_t height;
        uint32_t fps;
        uint32_t bitrate;
        enum class colorspace : uint32_t { yuv420 } colorspace;
    };
    parameter get(index index) const;

protected:
    object<void>::ptr impl_ptr;

    friend class ref_singleton<self>;
};

class ir_camera final : public object<ir_camera> {
public:
    using base = object<ir_camera>;
    using self = ir_camera;
    using ptr = std::shared_ptr<self>;

    struct init_parameter {};

    struct frame {
        std::vector<uint8_t> data;
        uint32_t width;
        uint32_t height;
    };

    enum class direction : uint32_t {
        left,
        right,
        front,
        back,
        top,
        bottom,
    };

    ir_camera(init_parameter const& parameter) {
        int32_t result{init(parameter)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(init_parameter const& parameter);

    int32_t subscribe(self::direction, std::function<void(frame&&)> const&);
    int32_t unsubscribe(self::direction);

    template <class... Args>
    static ptr make(Args&&... args) {
        return ref_singleton<self>::make(std::forward<Args>(args)...);
    }

protected:
    qlib::object<void>::ptr impl_ptr;

    friend class ref_singleton<self>;
};

};  // namespace psdk

// #ifdef DDS_IMPLEMENTATION
// namespace dds {

// template <>
// class type_factory<psdk::ir_camera::frame> : public type_factory<void> {
// public:
//     using base = type_factory<void>;
//     using self = type_factory<psdk::ir_camera::frame>;
//     using ptr = std::shared_ptr<self>;
//     using value_type = psdk::ir_camera::frame;

//     static DynamicType::_ref_type make_type() {
//         auto type_factory = DynamicTypeBuilderFactory::get_instance();

//         auto struct_descriptor = traits<TypeDescriptor>::make_shared();
//         struct_descriptor->name("struct");
//         struct_descriptor->kind(TK_STRUCTURE);
//         auto struct_builder = type_factory->create_type(struct_descriptor);

//         {
//             auto member_descriptor = traits<MemberDescriptor>::make_shared();
//             member_descriptor->name("data");
//             member_descriptor->type(
//                 type_factory
//                     ->create_sequence_type(type_factory->get_primitive_type(TK_UINT8),
//                                            static_cast<uint32_t>(LENGTH_UNLIMITED))
//                     ->build());
//             struct_builder->add_member(member_descriptor);
//         }

//         {
//             auto member_descriptor = traits<MemberDescriptor>::make_shared();
//             member_descriptor->name("width");
//             member_descriptor->type(type_factory->get_primitive_type(TK_UINT32));
//             struct_builder->add_member(member_descriptor);
//         }

//         {
//             auto member_descriptor = traits<MemberDescriptor>::make_shared();
//             member_descriptor->name("height");
//             member_descriptor->type(type_factory->get_primitive_type(TK_UINT32));
//             struct_builder->add_member(member_descriptor);
//         }

//         return struct_builder->build();
//     }

//     static value_type get(DynamicData::_ref_type const& data) {
//         value_type value{};
//         data->get_uint8_values(value.data, data->get_member_id_by_name("data"));
//         data->get_uint32_value(value.width, data->get_member_id_by_name("width"));
//         data->get_uint32_value(value.height, data->get_member_id_by_name("height"));
//         return value;
//     }

//     template <class _T>
//     static void set(DynamicData::_ref_type& data, _T&& value) {
//         data->set_uint8_values(data->get_member_id_by_name("data"), value.data);
//         data->set_uint32_value(data->get_member_id_by_name("width"), value.width);
//         data->set_uint32_value(data->get_member_id_by_name("height"), value.height);
//     }

//     static DynamicData::_ref_type make_data_ptr(DynamicType::_ref_type type) {
//         return base::make_data_ptr(type);
//     }

//     template <class _T>
//     static DynamicData::_ref_type make_data_ptr(DynamicType::_ref_type type, _T&& value) {
//         auto data_ptr = base::make_data_ptr(type);
//         set(data_ptr, std::forward<_T>(value));
//         return data_ptr;
//     }
// };

// };  // namespace dds
// #endif

};  // namespace qlib
