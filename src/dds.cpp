#include "qlib/dds.h"

#include <iostream>

#include "fastcdr/FastCdr.h"
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
#include "foonathan/memory/memory_pool.hpp"
#include "tinyxml2.h"

namespace qlib {

namespace dds {

using namespace eprosima::fastdds::dds;

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

template <>
struct convert<std::string> : public object {
    using value_type = std::string;

    static DynamicType::_ref_type make_type_ptr() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->create_string_type(static_cast<uint32_t>(LENGTH_UNLIMITED))
                ->build();
        });
    }

    static DynamicData::_ref_type make_data_ptr() {
        auto data_factory = DynamicDataFactory::get_instance();
        return data_factory->create_data(make_type_ptr());
    }

    static void set(DynamicData::_ref_type* data_ptr, value_type const& value) {
        (*data_ptr)->set_string_value(0u, value);
    }

    static void get(value_type* value_ptr, DynamicData::_ref_type const& data_ptr) {
        std::string str;
        data_ptr->get_string_value(str, 0u);
        *value_ptr = std::move(str);
    }
};

template <>
struct convert<string_t> : public object {
    using value_type = string_t;

    static DynamicType::_ref_type make_type_ptr() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->create_string_type(static_cast<uint32_t>(LENGTH_UNLIMITED))
                ->build();
        });
    }

    static DynamicData::_ref_type make_data_ptr() {
        auto data_factory = DynamicDataFactory::get_instance();
        return data_factory->create_data(make_type_ptr());
    }

    static void set(DynamicData::_ref_type* data_ptr, value_type const& value) {
        (*data_ptr)->set_string_value(0u, value.data());
    }

    static void get(value_type* value_ptr, DynamicData::_ref_type const& data_ptr) {
        std::string str;
        data_ptr->get_string_value(str, 0u);
        *value_ptr = str.c_str();
    }
};

#define REGISTER_TYPE(T, TK_ENUM)                                                                  \
    template <>                                                                                    \
    struct convert<T##_t> : public object {                                                        \
        using value_type = T##_t;                                                                  \
                                                                                                   \
        static DynamicType::_ref_type make_type_ptr() {                                            \
            return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {                  \
                return type_factory->get_primitive_type(TK_ENUM);                                  \
            });                                                                                    \
        }                                                                                          \
                                                                                                   \
        static DynamicData::_ref_type make_data_ptr() {                                            \
            auto data_factory = DynamicDataFactory::get_instance();                                \
            return data_factory->create_data(make_type_ptr());                                     \
        }                                                                                          \
                                                                                                   \
        static void set(DynamicData::_ref_type* data, value_type const& value) {                   \
            (*data)->set_##T##_value(0u, value);                                                   \
        }                                                                                          \
                                                                                                   \
        static void get(value_type* value, const DynamicData::_ref_type& data) {                   \
            data->get_##T##_value(*value, 0u);                                                     \
        }                                                                                          \
    };                                                                                             \
    template <>                                                                                    \
    struct convert<std::vector<T##_t>> : public object {                                           \
        using value_type = std::vector<T##_t>;                                                     \
                                                                                                   \
        static DynamicType::_ref_type make_type_ptr() {                                            \
            return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {                  \
                return type_factory                                                                \
                    ->create_sequence_type(type_factory->get_primitive_type(TK_ENUM),              \
                                           static_cast<uint32_t>(LENGTH_UNLIMITED))                \
                    ->build();                                                                     \
            });                                                                                    \
        }                                                                                          \
                                                                                                   \
        static DynamicData::_ref_type make_data_ptr() {                                            \
            auto data_factory = DynamicDataFactory::get_instance();                                \
            return data_factory->create_data(make_type_ptr());                                     \
        }                                                                                          \
                                                                                                   \
        static void set(DynamicData::_ref_type* data, value_type const& value) {                   \
            (*data)->set_##T##_values(0u, value);                                                  \
        }                                                                                          \
                                                                                                   \
        static void get(value_type* value, const DynamicData::_ref_type& data) {                   \
            data->get_##T##_values(*value, 0u);                                                    \
        }                                                                                          \
    };

REGISTER_TYPE(int8, TK_INT8);
REGISTER_TYPE(int16, TK_INT16);
REGISTER_TYPE(int32, TK_INT32);
REGISTER_TYPE(int64, TK_INT64);
REGISTER_TYPE(uint8, TK_UINT8);
REGISTER_TYPE(uint16, TK_UINT16);
REGISTER_TYPE(uint32, TK_UINT32);
REGISTER_TYPE(uint64, TK_UINT64);
REGISTER_TYPE(float32, TK_FLOAT32);
REGISTER_TYPE(float64, TK_FLOAT64);

#undef REGISTER_TYPE

struct publisher_impl : public object {
    std::unique_ptr<DomainParticipant, std::function<void(DomainParticipant*)>> participant;
    Publisher* publisher;
    Topic* topic;
    DataWriter* writer;
};

template <class T>
int32_t publisher::init(string_t const& topic) {
    int32_t result{0};

    do {
        auto impl_ptr = std::make_shared<publisher_impl>();

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

        auto type = TypeSupport{new DynamicPubSubType{convert<T>::make_type_ptr()}};
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
            impl_ptr->topic = impl_ptr->participant->create_topic(topic.data(), type.get_type_name(), qos);
            if (impl_ptr->topic == nullptr) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }
        }

        {
            DataWriterQos qos = DATAWRITER_QOS_DEFAULT;
            impl_ptr->publisher->get_default_datawriter_qos(qos);
            impl_ptr->writer = impl_ptr->publisher->create_datawriter(impl_ptr->topic, qos);
            if (impl_ptr->topic == nullptr) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }
        }

        self::impl = impl_ptr;
    } while (false);

    return result;
}

template <class T>
int32_t publisher::publish(T const& value) {
    int32_t result{0};

    do {
        auto impl_ptr = std::static_pointer_cast<publisher_impl>(self::impl);
        if (impl_ptr == nullptr) {
            result = static_cast<int32_t>(error::impl_nullptr);
            break;
        }

        auto data_ptr = convert<T>::make_data_ptr();
        convert<T>::set(&data_ptr, value);
        result = impl_ptr->writer->write(&data_ptr);
        if (result != 0) {
            result = static_cast<int32_t>(error::unknown);
            break;
        }
    } while (false);

    return result;
}

template <class T>
struct subscriber_impl : public object {
    class data_listener : public DataReaderListener {
    public:
        data_listener(subscriber_impl* _impl_ptr) : impl_ptr{_impl_ptr} {}

    protected:
        subscriber_impl* impl_ptr;

        void on_data_available(DataReader* reader) override {
            auto data_ptr = convert<T>::make_data_ptr();
            SampleInfo info;
            auto status = reader->take_next_sample(&data_ptr, &info);
            if (status == RETCODE_OK && info.valid_data && impl_ptr->callback != nullptr) {
                T value;
                convert<T>::get(&value, data_ptr);
                impl_ptr->callback(std::move(value));
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
    std::function<void(T&&)> callback;
};

template <class T>
int32_t subscriber::init(string_t const& topic, std::function<void(T&&)> const& callback) {
    int32_t result{0};

    do {
        auto impl_ptr = std::make_shared<subscriber_impl<T>>();

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

        auto type = TypeSupport{new DynamicPubSubType{convert<T>::make_type_ptr()}};
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
            impl_ptr->topic = impl_ptr->participant->create_topic(topic.data(), type.get_type_name(), qos);
            if (impl_ptr->topic == nullptr) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }
        }

        {
            DataReaderQos qos{DATAREADER_QOS_DEFAULT};
            impl_ptr->subscriber->get_default_datareader_qos(qos);
            impl_ptr->listener_ptr =
                std::make_shared<typename subscriber_impl<T>::data_listener>(impl_ptr.get());
            impl_ptr->reader = impl_ptr->subscriber->create_datareader(
                impl_ptr->topic, qos, impl_ptr->listener_ptr.get(), StatusMask::all());
            if (impl_ptr->topic == nullptr) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }
        }

        impl_ptr->callback = callback;

        self::impl = impl_ptr;
    } while (false);

    return result;
}

template int32_t publisher::publish<std::string>(std::string const&);
template int32_t publisher::publish<string_t>(string_t const&);
template int32_t publisher::publish<int8_t>(int8_t const&);
template int32_t publisher::publish<int16_t>(int16_t const&);
template int32_t publisher::publish<int32_t>(int32_t const&);
template int32_t publisher::publish<int64_t>(int64_t const&);
template int32_t publisher::publish<uint8_t>(uint8_t const&);
template int32_t publisher::publish<uint16_t>(uint16_t const&);
template int32_t publisher::publish<uint32_t>(uint32_t const&);
template int32_t publisher::publish<uint64_t>(uint64_t const&);
template int32_t publisher::publish<float32_t>(float32_t const&);
template int32_t publisher::publish<float64_t>(float64_t const&);
template int32_t publisher::publish<std::vector<int8_t>>(std::vector<int8_t> const&);
template int32_t publisher::publish<std::vector<int16_t>>(std::vector<int16_t> const&);
template int32_t publisher::publish<std::vector<int32_t>>(std::vector<int32_t> const&);
template int32_t publisher::publish<std::vector<int64_t>>(std::vector<int64_t> const&);
template int32_t publisher::publish<std::vector<uint8_t>>(std::vector<uint8_t> const&);
template int32_t publisher::publish<std::vector<uint16_t>>(std::vector<uint16_t> const&);
template int32_t publisher::publish<std::vector<uint32_t>>(std::vector<uint32_t> const&);
template int32_t publisher::publish<std::vector<uint64_t>>(std::vector<uint64_t> const&);
template int32_t publisher::publish<std::vector<float32_t>>(std::vector<float32_t> const&);
template int32_t publisher::publish<std::vector<float64_t>>(std::vector<float64_t> const&);

void __attribute__((used)) force_link_deps(void) {
    auto error = tinyxml2::XMLDocument::ErrorIDToName(tinyxml2::XML_SUCCESS);
    std::cout << error << std::endl;

    eprosima::fastcdr::FastBuffer fast_butter;
    std::cout << reinterpret_cast<void*>(&fast_butter) << std::endl;

    auto handler = foonathan::memory::out_of_memory::get_handler();
    std::cout << reinterpret_cast<void*>(&handler) << std::endl;
}

void (*g_dds)(void) = force_link_deps;

template <class T>
auto test() {
    auto publisher = publisher::make<T>("test");
    publisher->publish(T{});
    auto subscriber = subscriber::make<T>("test", [](T&& value) {});
}

auto register2 = []() {
    extern void (*g_dds)(void);
    THROW_EXCEPTION(force_link_deps, "g_dds is nullptr... ");
    if (g_dds == nullptr) {
        test<std::string>();
        test<string_t>();
        test<int8_t>();
        test<int16_t>();
        test<int32_t>();
        test<int64_t>();
        test<uint8_t>();
        test<uint16_t>();
        test<uint32_t>();
        test<uint64_t>();
        test<float32_t>();
        test<float64_t>();
        test<std::vector<int8_t>>();
        test<std::vector<int16_t>>();
        test<std::vector<int32_t>>();
        test<std::vector<int64_t>>();
        test<std::vector<uint8_t>>();
        test<std::vector<uint16_t>>();
        test<std::vector<uint32_t>>();
        test<std::vector<uint64_t>>();
        test<std::vector<float32_t>>();
        test<std::vector<float64_t>>();
    }
    return true;
}();

};  // namespace dds
};  // namespace qlib
