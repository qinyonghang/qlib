#pragma once

#define DDS_IMPLEMENTATION

#include <array>
#include <functional>
#include <vector>

#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/DataWriterListener.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/SampleInfo.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicPubSubType.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp"
#include "qlib/exception.h"
#include "qlib/object.h"

namespace qlib {

namespace dds {

using namespace eprosima::fastdds::dds;

auto register2 = []() {
    extern void (*g_dds)(void);
    THROW_EXCEPTION(g_dds, "g_dds is nullptr... ");
    return true;
}();

class publisher;
class subscriber;

class type : public object {
public:
    using self = type;
    using ptr = std::shared_ptr<self>;
    using base = object;

    virtual ~type() = default;

protected:
    type() = default;

    type(DynamicType::_ref_type type) {
        THROW_EXCEPTION(type, "type is nullptr...");
        self::_type = type;
        auto data_factory = DynamicDataFactory::get_instance();
        self::_data = data_factory->create_data(type);
    }

    type(self const&) = delete;
    type(self&&) = delete;

    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    friend class publisher;
    friend class subscriber;

protected:
    DynamicType::_ref_type _type{nullptr};
    DynamicData::_ref_type _data{nullptr};

    DynamicType::_ref_type make_type() { return self::_type; }
    DynamicData::_ref_type make_data_ptr() { return self::_data; }

    template <class Builder>
    static DynamicType::_ref_type create(Builder&& builder) {
        auto type_factory = DynamicTypeBuilderFactory::get_instance();

        auto struct_descriptor = traits<TypeDescriptor>::make_shared();
        struct_descriptor->name("struct");
        struct_descriptor->kind(TK_STRUCTURE);
        auto struct_builder = type_factory->create_type(struct_descriptor);

        auto member_descriptor = traits<MemberDescriptor>::make_shared();
        member_descriptor->name("impl");
        member_descriptor->type(builder(type_factory));
        struct_builder->add_member(member_descriptor);

        return struct_builder->build();
    }

    template <class T>
    static DynamicType::_ref_type make(DynamicTypeBuilderFactory::_ref_type type_factory);

    template <class T>
    T get() const;

    template <class T>
    void set(T const&);
};

#define REGISTER_TYPE(T, TK_TYPE)                                                                  \
    template <>                                                                                    \
    DynamicType::_ref_type type::make<T##_t>(DynamicTypeBuilderFactory::_ref_type type_factory) {  \
        return type_factory->get_primitive_type(TK_TYPE);                                          \
    }                                                                                              \
    template <>                                                                                    \
    std::vector<T##_t> type::get<std::vector<T##_t>>() const {                                     \
        std::vector<T##_t> value;                                                                  \
        self::_data->get_##T##_values(value, 0u);                                                  \
        return value;                                                                              \
    }                                                                                              \
    template <>                                                                                    \
    void type::set<std::vector<T##_t>>(std::vector<T##_t> const& value) {                          \
        self::_data->set_##T##_values(0u, value);                                                  \
    }

REGISTER_TYPE(int8, TK_INT8)
REGISTER_TYPE(uint8, TK_UINT8)
REGISTER_TYPE(int16, TK_INT16)
REGISTER_TYPE(uint16, TK_UINT16)
REGISTER_TYPE(int32, TK_INT32)
REGISTER_TYPE(uint32, TK_UINT32)
REGISTER_TYPE(int64, TK_INT64)
REGISTER_TYPE(uint64, TK_UINT64)
REGISTER_TYPE(float32, TK_FLOAT32)
REGISTER_TYPE(float64, TK_FLOAT64)

#undef REGISTER_TYPE

class string : public type {
public:
    using self = string;
    using ptr = std::shared_ptr<self>;
    using base = type;
    using value_type = base::string;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    string(self const&) = delete;
    string(self&&) = delete;

    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    string() : base{self::type} {}

    template <class String>
    string(String&& s) : base{self::type} {
        base::_data->set_string_value(0u, s);
    }

    self& operator=(value_type const& value) {
        base::_data->set_string_value(0u, value);
        return *this;
    }

    operator value_type() const {
        value_type value;
        base::_data->get_string_value(value, 0u);
        return value;
    }

    auto to_string() const { return static_cast<value_type>(*this); }

protected:
    static inline const DynamicType::_ref_type type{
        base::create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->create_string_type(static_cast<uint32_t>(LENGTH_UNLIMITED))
                ->build();
        })};
};

template <class T, uint32_t N = static_cast<uint32_t>(LENGTH_UNLIMITED)>
class sequence final : public type {
public:
    using self = sequence<T, N>;
    using ptr = std::shared_ptr<self>;
    using base = type;
    using value_type = std::vector<T>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    sequence(self const&) = delete;
    sequence(self&&) = delete;

    self& operator=(self const&) = delete;
    self& operator=(self&&) = delete;

    sequence() : base{self::type} {}

    sequence(value_type const& value) : base{self::type} { base::set<value_type>(value); }

    self& operator=(value_type const& value) {
        base::set<value_type>(value);
        return *this;
    }

    operator value_type() const { return base::get<value_type>(); }

    auto to_string() const { return static_cast<value_type>(*this); }

protected:
    static inline const DynamicType::_ref_type type{
        base::create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->create_sequence_type(base::make<T>(type_factory), N)->build();
        })};
};

#define REGISTER_TYPE(T, TK_TYPE)                                                                  \
    class T : public type {                                                                        \
    public:                                                                                        \
        using self = T;                                                                            \
        using ptr = std::shared_ptr<self>;                                                         \
        using base = type;                                                                         \
        using value_type = qlib::T##_t;                                                            \
                                                                                                   \
        template <class... Args>                                                                   \
        static ptr make(Args&&... args) {                                                          \
            return std::make_shared<self>(std::forward<Args>(args)...);                            \
        }                                                                                          \
                                                                                                   \
        T(self const&) = delete;                                                                   \
        T(self&&) = delete;                                                                        \
                                                                                                   \
        self& operator=(self const&) = delete;                                                     \
        self& operator=(self&&) = delete;                                                          \
                                                                                                   \
        T() : base(self::type) {}                                                                  \
                                                                                                   \
        T(value_type value) : base(self::type) {                                                   \
            base::_data->set_##T##_value(0u, value);                                               \
        }                                                                                          \
                                                                                                   \
        self& operator=(value_type value) {                                                        \
            base::_data->set_##T##_value(0u, value);                                               \
            return *this;                                                                          \
        }                                                                                          \
                                                                                                   \
        operator value_type() const {                                                              \
            value_type value;                                                                      \
            self::_data->get_##T##_value(value, 0u);                                               \
            return value;                                                                          \
        }                                                                                          \
                                                                                                   \
        auto to_string() const {                                                                   \
            return static_cast<value_type>(*this);                                                 \
        }                                                                                          \
                                                                                                   \
    protected:                                                                                     \
        static inline const DynamicType::_ref_type type{                                           \
            base::create([](DynamicTypeBuilderFactory::_ref_type type_factory) {                   \
                return type_factory->get_primitive_type(TK_TYPE);                                  \
            })};                                                                                   \
    };

REGISTER_TYPE(int8, TK_INT8)
REGISTER_TYPE(uint8, TK_UINT8)
REGISTER_TYPE(int16, TK_INT16)
REGISTER_TYPE(uint16, TK_UINT16)
REGISTER_TYPE(int32, TK_INT32)
REGISTER_TYPE(uint32, TK_UINT32)
REGISTER_TYPE(int64, TK_INT64)
REGISTER_TYPE(uint64, TK_UINT64)
REGISTER_TYPE(float32, TK_FLOAT32)
REGISTER_TYPE(float64, TK_FLOAT64)

#undef REGISTER_TYPE

class publisher : public object {
public:
    using self = publisher;
    using ptr = std::shared_ptr<self>;
    using base = object;

    static ptr make(dds::type::ptr const& type_ptr,
                    base::string const& topic,
                    DataWriterQos qos = DATAWRITER_QOS_DEFAULT) {
        return std::make_shared<self>(type_ptr, topic, qos);
    }

    template <class T>
    static ptr make(base::string const& topic, DataWriterQos qos = DATAWRITER_QOS_DEFAULT) {
        return std::make_shared<self>(topic, qos, std::in_place_type<T>);
    }

    publisher() = default;

    template <class T>
    publisher(base::string const& topic,
              DataWriterQos qos = DATAWRITER_QOS_DEFAULT,
              std::in_place_type_t<T> const& = std::in_place_type<T>) {
        int32_t result{init<T>(topic, qos)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    publisher(dds::type::ptr const& type_ptr,
              base::string const& topic,
              DataWriterQos qos = DATAWRITER_QOS_DEFAULT) {
        int32_t result{init(type_ptr, topic, qos)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    template <class T>
    int32_t init(base::string const& topic, DataWriterQos qos = DATAWRITER_QOS_DEFAULT) {
        return init(T::make(), topic, qos);
    }

    int32_t init(dds::type::ptr const& type_ptr,
                 base::string const& topic,
                 DataWriterQos qos = DATAWRITER_QOS_DEFAULT) {
        int32_t result{0};

        do {
            auto impl_ptr = std::make_shared<impl>();

            auto factory = DomainParticipantFactory::get_instance();
            impl_ptr->participant = decltype(impl_ptr->participant){
                factory->create_participant_with_default_profile(nullptr, StatusMask::none()),
                [](DomainParticipant* ptr) {
                    if (ptr != nullptr) {
                        ptr->delete_contained_entities();
                        auto factory = DomainParticipantFactory::get_instance();
                        factory->delete_participant(ptr);
                    }
                }};

            auto type = TypeSupport{new DynamicPubSubType{type_ptr->make_type()}};
            result = impl_ptr->participant->register_type(type);
            if (0 != result) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }

            {
                PublisherQos qos{PUBLISHER_QOS_DEFAULT};
                impl_ptr->participant->get_default_publisher_qos(qos);
                impl_ptr->publisher =
                    impl_ptr->participant->create_publisher(qos, nullptr, StatusMask::none());
                if (impl_ptr->publisher == nullptr) {
                    result = static_cast<int32_t>(error::unknown);
                    break;
                }
            }

            {
                TopicQos qos{TOPIC_QOS_DEFAULT};
                impl_ptr->participant->get_default_topic_qos(qos);
                impl_ptr->topic =
                    impl_ptr->participant->create_topic(topic, type.get_type_name(), qos);
                if (impl_ptr->topic == nullptr) {
                    result = static_cast<int32_t>(error::unknown);
                    break;
                }
            }

            {
                impl_ptr->publisher->get_default_datawriter_qos(qos);
                impl_ptr->writer = impl_ptr->publisher->create_datawriter(impl_ptr->topic, qos);
                if (impl_ptr->topic == nullptr) {
                    result = static_cast<int32_t>(error::unknown);
                    break;
                }
            }

            impl_ptr->type_ptr = type_ptr;

            self::impl_ptr = impl_ptr;
        } while (false);

        return result;
    }

    template <class T,
              class = std::enable_if_t<!std::is_convertible_v<std::decay_t<T>, dds::type::ptr>>>
    int32_t publish(T const& value);

    int32_t publish(dds::type::ptr const& type_ptr) {
        int32_t result{0};

        do {
            auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
            if (impl_ptr == nullptr) {
                result = static_cast<int32_t>(error::impl_nullptr);
                break;
            }

            auto data_ptr = type_ptr->make_data_ptr();
            result = impl_ptr->writer->write(&data_ptr);
            if (result != 0) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }
        } while (false);

        return result;
    }

protected:
    struct impl {
        std::unique_ptr<DomainParticipant, std::function<void(DomainParticipant*)>> participant;
        Publisher* publisher;
        Topic* topic;
        DataWriter* writer;
        dds::type::ptr type_ptr;
    };

    std::shared_ptr<impl> impl_ptr;
};

class subscriber : public object {
public:
    using base = object;
    using self = subscriber;
    using ptr = std::shared_ptr<self>;

    static ptr make(dds::type::ptr const& type_ptr,
                    base::string const& topic,
                    std::function<void(type::ptr const&)> const& callback) {
        return std::make_shared<self>(type_ptr, topic, callback);
    }

    template <class T,
              class = std::enable_if_t<!std::is_convertible_v<std::decay_t<T>, dds::type::ptr>>>
    static ptr make(base::string const& topic, std::function<void(T&&)> const& callback) {
        return std::make_shared<self>(topic, callback, std::in_place_type<T>);
    }

    template <class T, class = std::void_t<typename T::ptr>>
    static ptr make(base::string const& topic,
                    std::function<void(typename T::ptr const&)> const& callback) {
        return std::make_shared<self>(topic, callback, std::in_place_type<T>);
    }

    explicit subscriber() = default;

    template <class T,
              class = std::enable_if_t<!std::is_convertible_v<std::decay_t<T>, dds::type::ptr>>>
    explicit subscriber(base::string const& topic,
                        std::function<void(T&&)> const& callback,
                        std::in_place_type_t<T> const& = std::in_place_type<T>) {
        int32_t result{init<T>(topic, callback)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    template <class T, class = std::void_t<typename T::ptr>>
    explicit subscriber(base::string const& topic,
                        std::function<void(typename T::ptr const&)> const& callback,
                        std::in_place_type_t<T> const& = std::in_place_type<T>) {
        int32_t result{init<T>(topic, callback)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    subscriber(type::ptr const& type_ptr,
               base::string const& topic,
               std::function<void(type::ptr const&)> const& callback) {
        int32_t result{init(type_ptr, topic, callback)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    template <class T,
              class = std::enable_if_t<!std::is_convertible_v<std::decay_t<T>, dds::type::ptr>>>
    int32_t init(base::string const& topic, std::function<void(T&&)> const& callback);

    template <class T, class = std::void_t<typename T::ptr>>
    int32_t init(base::string const& topic,
                 std::function<void(typename T::ptr const&)> const& callback) {
        return init(T::make(), topic, [callback](type::ptr const& type_ptr) {
            callback(std::static_pointer_cast<T>(type_ptr));
        });
    }

    int32_t init(type::ptr const& type_ptr,
                 base::string const& topic,
                 std::function<void(type::ptr const&)> const& callback) {
        int32_t result{0};

        do {
            auto impl_ptr = std::make_shared<impl>();

            auto factory = DomainParticipantFactory::get_instance();
            impl_ptr->participant = decltype(impl_ptr->participant){
                factory->create_participant_with_default_profile(nullptr, StatusMask::none()),
                [](DomainParticipant* ptr) {
                    if (ptr != nullptr) {
                        ptr->delete_contained_entities();
                        auto factory = DomainParticipantFactory::get_instance();
                        factory->delete_participant(ptr);
                    }
                }};

            auto type = TypeSupport{new DynamicPubSubType{type_ptr->make_type()}};
            result = impl_ptr->participant->register_type(type);
            if (0 != result) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }

            SubscriberQos qos{SUBSCRIBER_QOS_DEFAULT};
            impl_ptr->participant->get_default_subscriber_qos(qos);
            impl_ptr->subscriber =
                impl_ptr->participant->create_subscriber(qos, nullptr, StatusMask::none());
            if (impl_ptr->subscriber == nullptr) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }

            {
                TopicQos qos{TOPIC_QOS_DEFAULT};
                impl_ptr->participant->get_default_topic_qos(qos);
                impl_ptr->topic =
                    impl_ptr->participant->create_topic(topic, type.get_type_name(), qos);
                if (impl_ptr->topic == nullptr) {
                    result = static_cast<int32_t>(error::unknown);
                    break;
                }
            }

            {
                DataReaderQos qos{DATAREADER_QOS_DEFAULT};
                impl_ptr->subscriber->get_default_datareader_qos(qos);
                impl_ptr->listener_ptr =
                    std::make_shared<typename impl::data_listener>(impl_ptr.get());
                impl_ptr->reader = impl_ptr->subscriber->create_datareader(
                    impl_ptr->topic, qos, impl_ptr->listener_ptr.get(), StatusMask::all());
                if (impl_ptr->topic == nullptr) {
                    result = static_cast<int32_t>(error::unknown);
                    break;
                }
            }

            impl_ptr->type_ptr = type_ptr;
            impl_ptr->callback = callback;

            self::impl_ptr = impl_ptr;
        } while (false);

        return result;
    }

protected:
    struct impl {
        class data_listener : public DataReaderListener {
        public:
            data_listener(impl* _impl_ptr) : impl_ptr{_impl_ptr} {}

        protected:
            impl* impl_ptr;

            void on_data_available(DataReader* reader) override {
                auto data_ptr = impl_ptr->type_ptr->make_data_ptr();
                SampleInfo info;
                auto status = reader->take_next_sample(&data_ptr, &info);
                if (status == RETCODE_OK && info.valid_data && impl_ptr->callback != nullptr) {
                    impl_ptr->callback(impl_ptr->type_ptr);
                }
            }

            void on_subscription_matched(DataReader* reader,
                                         SubscriptionMatchedStatus const& info) override {}
        };

        std::unique_ptr<DomainParticipant, std::function<void(DomainParticipant*)>> participant;
        Subscriber* subscriber;
        Topic* topic;
        DataReader* reader;
        std::shared_ptr<data_listener> listener_ptr;
        type::ptr type_ptr;
        std::function<void(type::ptr const&)> callback;
    };

    std::shared_ptr<impl> impl_ptr;
};

template <>
int32_t publisher::publish<publisher::base::string>(publisher::base::string const& value) {
    return publish(dds::string::make(value));
}

template <>
int32_t subscriber::init<subscriber::base::string>(
    base::string const& topic, std::function<void(base::string&&)> const& callback) {
    return init(dds::string::make(), topic, [callback](type::ptr const& type_ptr) {
        auto value_ptr = std::dynamic_pointer_cast<dds::string>(type_ptr);
        THROW_EXCEPTION(value_ptr, "value is {}!", typeid(*value_ptr).name());
        auto value = static_cast<base::string>(*value_ptr);
        callback(std::move(value));
    });
}

#define REGISTER_TYPE(T)                                                                           \
    template <>                                                                                    \
    int32_t publisher::publish<T##_t>(T##_t const& value) {                                        \
        return publish(dds::T::make(value));                                                       \
    }                                                                                              \
    template <>                                                                                    \
    int32_t publisher::publish<std::vector<T##_t>>(std::vector<T##_t> const& value) {              \
        return publish(dds::sequence<T##_t>::make(value));                                         \
    }                                                                                              \
    template <>                                                                                    \
    int32_t subscriber::init<T##_t>(base::string const& topic,                                     \
                                    std::function<void(T##_t&&)> const& callback) {                \
        return init(dds::T::make(), topic, [callback](type::ptr const& type_ptr) {                 \
            auto value_ptr = std::dynamic_pointer_cast<dds::T>(type_ptr);                          \
            THROW_EXCEPTION(value_ptr, "value is {}!", typeid(*value_ptr).name());                        \
            auto value = static_cast<T##_t>(*value_ptr);                                           \
            callback(std::move(value));                                                            \
        });                                                                                        \
    }                                                                                              \
    template <>                                                                                    \
    int32_t subscriber::init<std::vector<T##_t>>(                                                  \
        base::string const& topic, std::function<void(std::vector<T##_t>&&)> const& callback) {    \
        return init(dds::sequence<T##_t>::make(), topic, [callback](type::ptr const& type_ptr) {   \
            auto value_ptr = std::dynamic_pointer_cast<dds::sequence<T##_t>>(type_ptr);            \
            THROW_EXCEPTION(value_ptr, "value is {}!", typeid(*value_ptr).name());                        \
            auto value = static_cast<std::vector<T##_t>>(*value_ptr);                              \
            callback(std::move(value));                                                            \
        });                                                                                        \
    }

REGISTER_TYPE(int8)
REGISTER_TYPE(uint8)
REGISTER_TYPE(int16)
REGISTER_TYPE(uint16)
REGISTER_TYPE(int32)
REGISTER_TYPE(uint32)
REGISTER_TYPE(int64)
REGISTER_TYPE(uint64)
REGISTER_TYPE(float32)
REGISTER_TYPE(float64)

#undef REGISTER_TYPE

};  // namespace dds
};  // namespace qlib