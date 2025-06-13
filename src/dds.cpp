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

void __attribute__((used)) force_link_deps(void) {
    auto error = tinyxml2::XMLDocument::ErrorIDToName(tinyxml2::XML_SUCCESS);
    std::cout << error << std::endl;

    eprosima::fastcdr::FastBuffer fast_butter;
    std::cout << reinterpret_cast<void*>(&fast_butter) << std::endl;

    auto handler = foonathan::memory::out_of_memory::get_handler();
    std::cout << reinterpret_cast<void*>(&handler) << std::endl;
}

void (*g_dds)(void) = force_link_deps;
};  // namespace dds
};  // namespace qlib

namespace qlib {

namespace dds {

using namespace eprosima::fastdds::dds;

auto register2 = []() {
    extern void (*g_dds)(void);
    THROW_EXCEPTION(g_dds, "g_dds is nullptr... ");
    return true;
}();

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

struct convert : public object {
    static DynamicType::_ref_type make_type_ptr(type::ptr const& type) {
        DynamicType::_ref_type result{nullptr};

        if (type->value.type() == typeid(string_t)) {
            result = create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
                return type_factory->create_string_type(static_cast<uint32_t>(LENGTH_UNLIMITED))
                    ->build();
            });
        }

#define REGISTER_PRIMITIVE_TYPE(T, TK_ENUM)                                                        \
    else if (type->value.type() == typeid(T)) {                                                    \
        result = create([](DynamicTypeBuilderFactory::_ref_type type_factory) {                    \
            return type_factory->get_primitive_type(TK_ENUM);                                      \
        });                                                                                        \
    }

        REGISTER_PRIMITIVE_TYPE(int8_t, TK_INT8)
        REGISTER_PRIMITIVE_TYPE(int16_t, TK_INT16)
        REGISTER_PRIMITIVE_TYPE(int32_t, TK_INT32)
        REGISTER_PRIMITIVE_TYPE(int64_t, TK_INT64)
        REGISTER_PRIMITIVE_TYPE(uint8_t, TK_UINT8)
        REGISTER_PRIMITIVE_TYPE(uint16_t, TK_UINT16)
        REGISTER_PRIMITIVE_TYPE(uint32_t, TK_UINT32)
        REGISTER_PRIMITIVE_TYPE(uint64_t, TK_UINT64)

#undef REGISTER_PRIMITIVE_TYPE

#define REGISTER_VECTOR_TYPE(T, TK_ENUM)                                                           \
    else if (type->value.type() == typeid(std::vector<T>)) {                                       \
        result = create([](DynamicTypeBuilderFactory::_ref_type type_factory) {                    \
            return type_factory                                                                    \
                ->create_sequence_type(type_factory->get_primitive_type(TK_ENUM),                  \
                                       static_cast<uint32_t>(LENGTH_UNLIMITED))                    \
                ->build();                                                                         \
        });                                                                                        \
    }

        REGISTER_VECTOR_TYPE(int8_t, TK_INT8)
        REGISTER_VECTOR_TYPE(int16_t, TK_INT16)
        REGISTER_VECTOR_TYPE(int32_t, TK_INT32)
        REGISTER_VECTOR_TYPE(int64_t, TK_INT64)
        REGISTER_VECTOR_TYPE(uint8_t, TK_UINT8)
        REGISTER_VECTOR_TYPE(uint16_t, TK_UINT16)
        REGISTER_VECTOR_TYPE(uint32_t, TK_UINT32)
        REGISTER_VECTOR_TYPE(uint64_t, TK_UINT64)

#undef REGISTER_VECTOR_TYPE

        THROW_EXCEPTION(result != nullptr, "type not found!");
        return result;
    }

    static DynamicData::_ref_type make_data_ptr(type::ptr const& type) {
        auto data_factory = DynamicDataFactory::get_instance();
        return data_factory->create_data(make_type_ptr(type));
    }

    static void set(DynamicData::_ref_type* data_ptr, type::ptr const& type) {
        if (type->value.type() == typeid(string_t)) {
            (*data_ptr)->set_string_value(0u, std::any_cast<string_t>(type->value));
        }

#define HANDLE_PRIMITIVE_TYPE(T, SETTER)                                                           \
    else if (type->value.type() == typeid(T)) {                                                    \
        (*data_ptr)->SETTER(0u, std::any_cast<T>(type->value));                                    \
    }

        HANDLE_PRIMITIVE_TYPE(int8_t, set_int8_value)
        HANDLE_PRIMITIVE_TYPE(int16_t, set_int16_value)
        HANDLE_PRIMITIVE_TYPE(int32_t, set_int32_value)
        HANDLE_PRIMITIVE_TYPE(int64_t, set_int64_value)

        HANDLE_PRIMITIVE_TYPE(uint8_t, set_uint8_value)
        HANDLE_PRIMITIVE_TYPE(uint16_t, set_uint16_value)
        HANDLE_PRIMITIVE_TYPE(uint32_t, set_uint32_value)
        HANDLE_PRIMITIVE_TYPE(uint64_t, set_uint64_value)

#undef HANDLE_PRIMITIVE_TYPE

#define HANDLE_VECTOR_TYPE(T, SETTER)                                                              \
    else if (type->value.type() == typeid(std::vector<T>)) {                                       \
        (*data_ptr)->SETTER(0u, std::any_cast<std::vector<T>>(type->value));                       \
    }

        HANDLE_VECTOR_TYPE(int8_t, set_int8_values)
        HANDLE_VECTOR_TYPE(int16_t, set_int16_values)
        HANDLE_VECTOR_TYPE(int32_t, set_int32_values)
        HANDLE_VECTOR_TYPE(int64_t, set_int64_values)

        HANDLE_VECTOR_TYPE(uint8_t, set_uint8_values)
        HANDLE_VECTOR_TYPE(uint16_t, set_uint16_values)
        HANDLE_VECTOR_TYPE(uint32_t, set_uint32_values)
        HANDLE_VECTOR_TYPE(uint64_t, set_uint64_values)

#undef HANDLE_VECTOR_TYPE

        else {
            THROW_EXCEPTION(False, "type not found!");
        }
    }

    static void get(type::ptr* type, DynamicData::_ref_type const& data_ptr) {
        auto& value = (*type)->value;

        if (value.type() == typeid(string_t)) {
            string_t str;
            data_ptr->get_string_value(str, 0u);
            value = str;
        }

#define HANDLE_PRIMITIVE_GET(T, GETTER)                                                            \
    else if (value.type() == typeid(T)) {                                                          \
        T val;                                                                                     \
        data_ptr->GETTER(val, 0u);                                                                 \
        value = val;                                                                               \
    }

        HANDLE_PRIMITIVE_GET(int8_t, get_int8_value)
        HANDLE_PRIMITIVE_GET(int16_t, get_int16_value)
        HANDLE_PRIMITIVE_GET(int32_t, get_int32_value)
        HANDLE_PRIMITIVE_GET(int64_t, get_int64_value)

        HANDLE_PRIMITIVE_GET(uint8_t, get_uint8_value)
        HANDLE_PRIMITIVE_GET(uint16_t, get_uint16_value)
        HANDLE_PRIMITIVE_GET(uint32_t, get_uint32_value)
        HANDLE_PRIMITIVE_GET(uint64_t, get_uint64_value)

#undef HANDLE_PRIMITIVE_GET

#define HANDLE_VECTOR_GET(T, GETTER)                                                               \
    else if (value.type() == typeid(std::vector<T>)) {                                             \
        std::vector<T> vec;                                                                        \
        data_ptr->GETTER(vec, 0u);                                                                 \
        value = vec;                                                                               \
    }

        HANDLE_VECTOR_GET(int8_t, get_int8_values)
        HANDLE_VECTOR_GET(int16_t, get_int16_values)
        HANDLE_VECTOR_GET(int32_t, get_int32_values)
        HANDLE_VECTOR_GET(int64_t, get_int64_values)

        HANDLE_VECTOR_GET(uint8_t, get_uint8_values)
        HANDLE_VECTOR_GET(uint16_t, get_uint16_values)
        HANDLE_VECTOR_GET(uint32_t, get_uint32_values)
        HANDLE_VECTOR_GET(uint64_t, get_uint64_values)

#undef HANDLE_VECTOR_GET

        else {
            THROW_EXCEPTION(false, "unsupported type: {}", value.type().name());
        }
    }
};

struct publisher_impl : public object {
    std::unique_ptr<DomainParticipant, std::function<void(DomainParticipant*)>> participant;
    Publisher* publisher;
    Topic* topic;
    DataWriter* writer;
    dds::type::ptr type_ptr;
};

int32_t publisher::init(dds::type::ptr const& type_ptr, string_t const& topic) {
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

        auto type = TypeSupport{new DynamicPubSubType{convert::make_type_ptr(type_ptr)}};
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
            impl_ptr->topic = impl_ptr->participant->create_topic(topic, type.get_type_name(), qos);
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

        impl_ptr->type_ptr = type_ptr;

        self::impl = impl_ptr;
    } while (false);

    return result;
}

int32_t publisher::publish(dds::type::ptr const& type_ptr) {
    int32_t result{0};

    do {
        auto impl_ptr = std::static_pointer_cast<publisher_impl>(self::impl);
        if (impl_ptr == nullptr) {
            result = static_cast<int32_t>(error::impl_nullptr);
            break;
        }

        auto data_ptr = convert::make_data_ptr(type_ptr);
        convert::set(&data_ptr, type_ptr);
        result = impl_ptr->writer->write(&data_ptr);
        if (result != 0) {
            result = static_cast<int32_t>(error::unknown);
            break;
        }
    } while (false);

    return result;
}

struct subscriber_impl : public object {
    class data_listener : public DataReaderListener {
    public:
        data_listener(subscriber_impl* _impl_ptr) : impl_ptr{_impl_ptr} {}

    protected:
        subscriber_impl* impl_ptr;

        void on_data_available(DataReader* reader) override {
            auto data_ptr = convert::make_data_ptr(impl_ptr->type_ptr);
            SampleInfo info;
            auto status = reader->take_next_sample(&data_ptr, &info);
            if (status == RETCODE_OK && info.valid_data && impl_ptr->callback != nullptr) {
                convert::get(&impl_ptr->type_ptr, data_ptr);
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

int32_t subscriber::init(type::ptr const& type_ptr,
                         string_t const& topic,
                         std::function<void(type::ptr const&)> const& callback) {
    int32_t result{0};

    do {
        auto impl_ptr = std::make_shared<subscriber_impl>();

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

        auto type = TypeSupport{new DynamicPubSubType{convert::make_type_ptr(type_ptr)}};
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
            impl_ptr->topic = impl_ptr->participant->create_topic(topic, type.get_type_name(), qos);
            if (impl_ptr->topic == nullptr) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }
        }

        {
            DataReaderQos qos{DATAREADER_QOS_DEFAULT};
            impl_ptr->subscriber->get_default_datareader_qos(qos);
            impl_ptr->listener_ptr =
                std::make_shared<typename subscriber_impl::data_listener>(impl_ptr.get());
            impl_ptr->reader = impl_ptr->subscriber->create_datareader(
                impl_ptr->topic, qos, impl_ptr->listener_ptr.get(), StatusMask::all());
            if (impl_ptr->topic == nullptr) {
                result = static_cast<int32_t>(error::unknown);
                break;
            }
        }

        impl_ptr->type_ptr = type_ptr;
        impl_ptr->callback = callback;

        self::impl = impl_ptr;
    } while (false);

    return result;
}

};  // namespace dds
};  // namespace qlib
