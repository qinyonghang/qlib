#pragma once

#define PSDK_IMPLEMENTATION

#include <functional>
#include <future>
#include <vector>

#include "qlib/exception.h"
#include "qlib/object.h"
#include "qlib/singleton.h"

namespace qlib {

namespace psdk {

struct init_parameter {
    string_t log_prefix{"logs/psdk"};

    uint32_t connect_type;
    string_t app_name;
    string_t app_id;
    string_t app_key;
    string_t app_license;
    string_t developer_account;
    string_t baud_rate;

    // uart
    string_t uart1;
    string_t uart2;

    // network
    struct {
        string_t name;
        uint32_t vid;
        uint32_t pid;

        auto to_string() const { return fmt::format("[{},{},{}]", name, vid, pid); }
    } network;

    // usb
    struct {
        uint32_t vid;
        uint32_t pid;
        struct {
            string_t ep_in;
            string_t ep_out;
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

object::ptr make();
object::ptr make(init_parameter const&);

class flight_control final : public object {
public:
    using base = object;
    using self = flight_control;
    using ptr = std::shared_ptr<self>;

    template <class... Args>
    static ptr make(Args&&... args) {
        ptr result{nullptr};

        ptr ptr = ref_singleton<self>::make();
        if (ptr->init(std::forward<Args>(args)...) == 0) {
            result = ptr;
        }
        return result;
    }

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

    struct waypoints_parameter : public parameter {
        using self = waypoints_parameter;
        using ptr = std::shared_ptr<self>;

        std::vector<position> points;
        action finish_action{action::no_action};
        uint32_t repeat_times{0u};

        auto to_string() const {
            string_t out;
            for (auto& point : points) {
                out += fmt::format("[{},{},{}]", point.longitude, point.latitude,
                                   point.relative_height);
            }
            return fmt::format("[[{}],{},{}]", out, static_cast<uint32_t>(finish_action),
                               repeat_times);
        }
    };

    flight_control() = default;

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
    object::ptr impl_ptr;

    friend class ref_singleton<self>;
};

class autoland : public object {
public:
    using self = autoland;
    using base = object;
    using ptr = base::sptr<self>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return ref_singleton<self>::make(std::forward<Args>(args)...);
    }

    // autoland() = default;

    template <class... Args>
    autoland(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    struct init_parameter : public base::parameter {};

    int32_t init(init_parameter const&);

    int32_t init() { return init(init_parameter{}); }

    struct start_parameter : public base::parameter {
        bool_t enable_vision{True};
        string_t codes{"Tag16h5"};
        std::function<void(int32_t)> callback{nullptr};

        auto to_string() const -> string_t {
            return fmt::format("[{},{},{}]", enable_vision, codes, static_cast<bool>(callback));
        }
    };
    int32_t start(start_parameter const&);

    template <class Callback,
              class = std::enable_if_t<
                  std::is_convertible_v<std::decay_t<Callback>, std::function<void(int32_t)>>>>
    int32_t start(Callback&& callback) {
        return start(start_parameter{
            .enable_vision = True,
            .codes = "Tag16h5",
            .callback = std::forward<Callback>(callback),
        });
    }

    int32_t start() {
        auto p = std::make_shared<std::promise<int32_t>>();
        auto f = p->get_future();

        int32_t result{start([p](int32_t result) { p->set_value(result); })};
        if (result == 0) {
            result = f.get();
        }

        return result;
    }

    struct stop_parameter : public base::parameter {
        std::function<void(int32_t)> callback{nullptr};

        auto to_string() const -> string_t {
            return fmt::format("[{}]", static_cast<bool>(callback));
        }
    };
    int32_t stop(stop_parameter const&);

protected:
    object::ptr impl;
};

class camera final : public object {
public:
    using base = object;
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
    object::ptr impl_ptr;

    friend class ref_singleton<self>;
};

class ir_camera final : public object {
public:
    using base = object;
    using self = ir_camera;
    using ptr = std::shared_ptr<self>;

    struct init_parameter {};

    enum class index : uint32_t {
        left,
        right,
        front,
        back,
        top,
        bottom,
    };

    template <class... Args>
    static ptr make(Args&&... args) {
        return ref_singleton<self>::make(std::forward<Args>(args)...);
    }

    template <class... Args>
    ir_camera(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(init_parameter const& parameter);
    int32_t init() { return init(init_parameter{}); }

    struct frame {
        std::vector<uint8_t> data;
        bool_t left;
    };

    int32_t subscribe(index, std::function<void(frame&&)> const&);
    int32_t unsubscribe(index);

    struct parameter : public base::parameter {
        uint32_t width{0u};
        uint32_t height{0u};
        std::array<float32_t, 9u> intrinsics_left, intrinsics_right;
        std::array<float32_t, 9u> rotation_left_in_right;
        std::array<float32_t, 3u> translation_left_in_right;

        auto to_string() const -> string_t { return fmt::format("[{},{}]", width, height); }
    };
    parameter get(index index) const;

protected:
    object::ptr impl_ptr;

    friend class ref_singleton<self>;
};

class data_subcriber final : public object {
public:
    using self = data_subcriber;
    using ptr = sptr<self>;

    struct data final : public object {
        uint32_t status;
        uint32_t mode;
        std::array<float64_t, 3> velocity;
        std::array<float64_t, 3> euler_angles;
        float64_t altitude;
        std::array<float64_t, 3> position{0., 0., 0.};
        // std::array<float64_t, 3> rtk_position{0., 0., 0.};
        size_t number_of_satellites;
        std::array<float64_t, 3> compass;
        std::array<float64_t, 3> battery;

        auto to_string() {
            return fmt::format("[status={}, mode={}, velocity=[{},{},{}], euler_angles=[{},{},{}], "
                               "altitude={}, position=[{},{},{}], number_of_satellites={}, "
                               "compass=[{},{},{}], battery=[{},{},{}]]",
                               status, mode, velocity[0], velocity[1], velocity[2], euler_angles[0],
                               euler_angles[1], euler_angles[2], altitude, position[0], position[1],
                               position[2], number_of_satellites, compass[0], compass[1],
                               compass[2], battery[0], battery[1], battery[2]);
        }
    };

    data get() const;

    template <class... Args>
    static ptr make(Args&&... args) {
        ptr result{nullptr};

        ptr ptr = ref_singleton<self>::make();
        if (ptr->init(std::forward<Args>(args)...) == 0) {
            result = ptr;
        }
        return result;
    }

    data_subcriber() = default;
    int32_t init(object::ptr const&);

protected:
    object::ptr impl;

    friend class ref_singleton<self>;
};

};  // namespace psdk

};  // namespace qlib
