#pragma once

#include <functional>

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
#include "qlib/logger.h"
#include "qlib/object.h"

namespace qlib {

namespace dds {

using namespace eprosima::fastdds::dds;

template <typename T, class = void>
class publisher final : public qlib::object<publisher<T>> {
protected:
    struct impl {
        std::unique_ptr<DomainParticipant, std::function<void(DomainParticipant*)>> participant;
        Publisher* publisher;
        Topic* topic;
        DataWriter* writer;
        std::function<DynamicData::_ref_type(T const&)> allocator;
    };

    std::shared_ptr<impl> __impl_ptr;

public:
    using base = qlib::object<publisher<T>>;
    using self = publisher<T>;
    using ptr = std::shared_ptr<self>;

    publisher() = default;

    template <typename... Args>
    publisher(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(string const& topic,
                 eprosima::fastdds::dds::TypeSupport type,
                 std::function<DynamicData::_ref_type(T const&)> allocator = nullptr) {
        int32_t result{0};

        do {
            extern void (*g_dds)(void);
            THROW_EXCEPTION(g_dds, "g_dds is nullptr... ");

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

            result = type.register_type(impl_ptr->participant.get());
            if (0 != result) {
                qError("Register Type Failed!");
                result = UNKNOWN_ERROR;
                break;
            }

            PublisherQos qos{PUBLISHER_QOS_DEFAULT};
            impl_ptr->participant->get_default_publisher_qos(qos);
            impl_ptr->publisher =
                impl_ptr->participant->create_publisher(qos, nullptr, StatusMask::none());
            if (impl_ptr->publisher == nullptr) {
                qError("Create Publisher Failed!");
                result = UNKNOWN_ERROR;
                break;
            }

            {
                TopicQos qos{TOPIC_QOS_DEFAULT};
                impl_ptr->participant->get_default_topic_qos(qos);
                impl_ptr->topic =
                    impl_ptr->participant->create_topic(topic, type.get_type_name(), qos);
                if (impl_ptr->topic == nullptr) {
                    qError("Create Topic Failed!");
                    result = UNKNOWN_ERROR;
                    break;
                }
            }

            {
                DataWriterQos qos{DATAWRITER_QOS_DEFAULT};
                impl_ptr->publisher->get_default_datawriter_qos(qos);
                impl_ptr->writer = impl_ptr->publisher->create_datawriter(impl_ptr->topic, qos);
                if (impl_ptr->topic == nullptr) {
                    qError("Create Writer Failed!");
                    result = UNKNOWN_ERROR;
                    break;
                }
            }

            impl_ptr->allocator = allocator;

            __impl_ptr = impl_ptr;
        } while (false);

        return result;
    }

    int32_t init(string const& topic) {
        static_assert(std::is_same_v<T, void>, "Unsupported Implementation!");
        return -1;
    }

    int32_t publish(T const& data) {
        int32_t result{0};

        do {
            auto impl_ptr = std::static_pointer_cast<impl>(__impl_ptr);

            if (impl_ptr == nullptr) {
                qError("Impl is nullptr!");
                result = IMPL_NULLPTR;
                break;
            }

            if (impl_ptr->allocator != nullptr) {
                auto data_ptr = impl_ptr->allocator(data);
                result = impl_ptr->writer->write(&data_ptr);
            } else {
                result = impl_ptr->writer->write(&data);
            }
            if (result != 0) {
                qError("Publish Failed!");
                result = UNKNOWN_ERROR;
                break;
            }
        } while (false);

        return result;
    }
};

template <typename T, class = void>
class subscriber final : public qlib::object<subscriber<T>> {
protected:
    struct impl {
        std::unique_ptr<DomainParticipant, std::function<void(DomainParticipant*)>> participant;
        eprosima::fastdds::dds::TypeSupport type;
        Subscriber* subscriber;
        Topic* topic;
        DataReader* reader;
        std::function<void(T&&)> callback;
        std::function<ReturnCode_t(T*, SampleInfo* info, DataReader* reader)> allocator;

        struct : public DataReaderListener {
            impl* impl_ptr;

            void on_data_available(DataReader* reader) override {
                if (impl_ptr != nullptr && reader != nullptr) {
                    T data;
                    SampleInfo info;
                    ReturnCode_t status;
                    if (impl_ptr->allocator != nullptr) {
                        status = impl_ptr->allocator(&data, &info, reader);
                    } else {
                        status = reader->take_next_sample(&data, &info);
                    }
                    if (status == RETCODE_OK && impl_ptr->callback != nullptr && info.valid_data) {
                        impl_ptr->callback(std::move(data));
                    }
                }
            }

            // void on_subscription_matched(DataReader* reader,
            //                              SubscriptionMatchedStatus const& info) override {
            //     qTrace("Subscription Matched! Count: {}, Total Count: {}", info.current_count,
            //            info.total_count);
            // }
        } reader_listener;
    };

    std::shared_ptr<impl> __impl_ptr;

public:
    using base = qlib::object<subscriber>;
    using self = subscriber;
    using ptr = std::shared_ptr<self>;

    subscriber() = default;

    template <typename... Args>
    subscriber(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    int32_t init(
        string const& topic,
        eprosima::fastdds::dds::TypeSupport type,
        std::function<ReturnCode_t(T*, SampleInfo* info, DataReader* reader)> allocator = nullptr) {
        int32_t result{0};

        do {
            extern void (*g_dds)(void);
            THROW_EXCEPTION(g_dds, "g_dds is nullptr... ");

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

            impl_ptr->type = type;
            result = impl_ptr->type.register_type(impl_ptr->participant.get());
            if (0 != result) {
                qError("Register Type Failed!");
                result = UNKNOWN_ERROR;
                break;
            }

            SubscriberQos qos{SUBSCRIBER_QOS_DEFAULT};
            impl_ptr->participant->get_default_subscriber_qos(qos);
            impl_ptr->subscriber =
                impl_ptr->participant->create_subscriber(qos, nullptr, StatusMask::none());
            if (impl_ptr->subscriber == nullptr) {
                qError("Create Subscriber Failed!");
                result = UNKNOWN_ERROR;
                break;
            }

            {
                TopicQos qos{TOPIC_QOS_DEFAULT};
                impl_ptr->participant->get_default_topic_qos(qos);
                impl_ptr->topic =
                    impl_ptr->participant->create_topic(topic, type.get_type_name(), qos);
                if (impl_ptr->topic == nullptr) {
                    qError("Create Topic Failed!");
                    result = UNKNOWN_ERROR;
                    break;
                }
            }

            {
                DataReaderQos qos{DATAREADER_QOS_DEFAULT};
                impl_ptr->subscriber->get_default_datareader_qos(qos);
                impl_ptr->reader_listener.impl_ptr = impl_ptr.get();
                impl_ptr->reader = impl_ptr->subscriber->create_datareader(
                    impl_ptr->topic, qos, &impl_ptr->reader_listener, StatusMask::all());
                if (impl_ptr->topic == nullptr) {
                    qError("Create Writer Failed!");
                    result = UNKNOWN_ERROR;
                    break;
                }
            }

            impl_ptr->allocator = allocator;

            __impl_ptr = impl_ptr;
        } while (false);

        return result;
    }

    int32_t init(string const& topic) {
        static_assert(std::is_same_v<T, void>, "Unsupported Implementation!");
        return -1;
    }

    template <class Callback>
    int32_t subscribe(Callback&& callback) {
        int32_t result{0};

        do {
            auto impl_ptr = std::static_pointer_cast<impl>(__impl_ptr);
            if (impl_ptr == nullptr) {
                qError("Impl is nullptr!");
                result = IMPL_NULLPTR;
                break;
            }

            impl_ptr->callback = std::forward<Callback>(callback);
        } while (false);

        return result;
    }
};

class dynamic_type_factory final : public qlib::object<dynamic_type_factory> {
public:
    using base = qlib::object<dynamic_type_factory>;
    using self = dynamic_type_factory;
    using ptr = std::shared_ptr<self>;

    template <class T>
    static DynamicType::_ref_type create() {
        static_assert(std::is_same_v<T, void>, "Unsupported Implementation!");
        return nullptr;
    }

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

    static DynamicType::_ref_type string() {
        auto type_factory = DynamicTypeBuilderFactory::get_instance();

        auto struct_descriptor = traits<TypeDescriptor>::make_shared();
        struct_descriptor->name("struct");
        struct_descriptor->kind(TK_STRUCTURE);
        auto struct_builder = type_factory->create_type(struct_descriptor);

        auto member_descriptor = traits<MemberDescriptor>::make_shared();
        member_descriptor->name("impl");
        member_descriptor->type(
            type_factory->create_string_type(static_cast<uint32_t>(LENGTH_UNLIMITED))->build());
        struct_builder->add_member(member_descriptor);

        return struct_builder->build();
    }

    static DynamicType::_ref_type int8() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_INT8);
        });
    }

    static DynamicType::_ref_type uint8() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_UINT8);
        });
    }

    static DynamicType::_ref_type int16() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_INT16);
        });
    }

    static DynamicType::_ref_type uint16() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_UINT16);
        });
    }

    static DynamicType::_ref_type int32() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_INT32);
        });
    }

    static DynamicType::_ref_type uint32() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_UINT32);
        });
    }

    static DynamicType::_ref_type int64() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_INT64);
        });
    }

    static DynamicType::_ref_type uint64() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_UINT64);
        });
    }

    static DynamicType::_ref_type float32() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_FLOAT32);
        });
    }

    static DynamicType::_ref_type float64() {
        return create([](DynamicTypeBuilderFactory::_ref_type type_factory) {
            return type_factory->get_primitive_type(TK_FLOAT64);
        });
    }
};

template <>
int32_t publisher<string>::init(string const& topic) {
    auto type = dynamic_type_factory::string();
    return init(topic, TypeSupport{new DynamicPubSubType(type)}, [type](string const& data) {
        auto data_factory = DynamicDataFactory::get_instance();
        auto data_ptr = data_factory->create_data(type);
        data_ptr->set_string_value(data_ptr->get_member_id_by_name("impl"), data);
        return data_ptr;
    });
}

template <>
int32_t subscriber<string>::init(string const& topic) {
    auto type = dynamic_type_factory::string();
    return init(
        topic, TypeSupport{new DynamicPubSubType(type)},
        [type](string* _data_ptr, SampleInfo* info_ptr, DataReader* reader) -> ReturnCode_t {
            auto data_factory = DynamicDataFactory::get_instance();
            auto data_ptr = data_factory->create_data(type);
            auto status = reader->take_next_sample(&data_ptr, info_ptr);
            if (status == RETCODE_OK && info_ptr->valid_data) {
                data_ptr->get_string_value(*_data_ptr, data_ptr->get_member_id_by_name("impl"));
            }
            return status;
        });
}

template <class T>
class publisher<T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>> final
        : public publisher<T> {
public:
    int32_t init(string const& topic) {
        DynamicType::_ref_type type;

        if constexpr (std::is_same_v<T, int8_t>) {
            type = dynamic_type_factory::int8();
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            type = dynamic_type_factory::uint8();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            type = dynamic_type_factory::int16();
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            type = dynamic_type_factory::uint16();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            type = dynamic_type_factory::int32();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            type = dynamic_type_factory::uint32();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            type = dynamic_type_factory::int64();
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            type = dynamic_type_factory::uint64();
        } else if constexpr (std::is_same_v<T, float32_t>) {
            type = dynamic_type_factory::float32();
        } else if constexpr (std::is_same_v<T, float64_t>) {
            type = dynamic_type_factory::float64();
        } else {
            static_assert(std::is_same_v<T, void>, "Unsupported Implementation!");
        }

        return init(topic, TypeSupport{new DynamicPubSubType(type)}, [type](T data) {
            auto data_factory = DynamicDataFactory::get_instance();
            auto data_ptr = data_factory->create_data(type);
            auto member_id = data_ptr->get_member_id_by_name("impl");
            if constexpr (std::is_same_v<T, int8_t>) {
                data_ptr->set_int8_value(member_id, data);
            } else if constexpr (std::is_same_v<T, uint8_t>) {
                data_ptr->set_uint8_value(member_id, data);
            } else if constexpr (std::is_same_v<T, int16_t>) {
                data_ptr->set_int16_value(member_id, data);
            } else if constexpr (std::is_same_v<T, uint16_t>) {
                data_ptr->set_uint16_value(member_id, data);
            } else if constexpr (std::is_same_v<T, int32_t>) {
                data_ptr->set_int32_value(member_id, data);
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                data_ptr->set_uint32_value(member_id, data);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                data_ptr->set_int64_value(member_id, data);
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                data_ptr->set_uint64_value(member_id, data);
            } else if constexpr (std::is_same_v<T, float32_t>) {
                data_ptr->set_float32_value(member_id, data);
            } else if constexpr (std::is_same_v<T, float64_t>) {
                data_ptr->set_float64_value(member_id, data);
            } else {
                static_assert(std::is_same_v<T, void>, "Unsupported Implementation!");
            }
            return data_ptr;
        });
    }
};

template <class T>
class subscriber<T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>> final
        : public subscriber<T> {
public:
    int32_t init(string const& topic) {
        DynamicType::_ref_type type;

        if constexpr (std::is_same_v<T, int8_t>) {
            type = dynamic_type_factory::int8();
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            type = dynamic_type_factory::uint8();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            type = dynamic_type_factory::int16();
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            type = dynamic_type_factory::uint16();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            type = dynamic_type_factory::int32();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            type = dynamic_type_factory::uint32();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            type = dynamic_type_factory::int64();
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            type = dynamic_type_factory::uint64();
        } else if constexpr (std::is_same_v<T, float32_t>) {
            type = dynamic_type_factory::float32();
        } else if constexpr (std::is_same_v<T, float64_t>) {
            type = dynamic_type_factory::float64();
        } else {
            static_assert(std::is_same_v<T, void>, "Unsupported Implementation!");
        }

        return init(topic, TypeSupport{new DynamicPubSubType(type)},
                    [type](T* _data_ptr, SampleInfo* info_ptr, DataReader* reader) -> ReturnCode_t {
                        auto data_factory = DynamicDataFactory::get_instance();
                        auto data_ptr = data_factory->create_data(type);
                        auto status = reader->take_next_sample(&data_ptr, info_ptr);
                        if (status == RETCODE_OK && info_ptr->valid_data) {
                            auto member_id = data_ptr->get_member_id_by_name("impl");
                            if constexpr (std::is_same_v<T, int8_t>) {
                                data_ptr->get_int8_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, uint8_t>) {
                                data_ptr->get_uint8_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, int16_t>) {
                                data_ptr->get_int16_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, uint16_t>) {
                                data_ptr->get_uint16_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, int32_t>) {
                                data_ptr->get_int32_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, uint32_t>) {
                                data_ptr->get_uint32_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, int64_t>) {
                                data_ptr->get_int64_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, uint64_t>) {
                                data_ptr->get_uint64_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, float32_t>) {
                                data_ptr->get_float32_value(*_data_ptr, member_id);
                            } else if constexpr (std::is_same_v<T, float64_t>) {
                                data_ptr->get_float64_value(*_data_ptr, member_id);
                            } else {
                                static_assert(std::is_same_v<T, void>,
                                              "Unsupported Implementation!");
                            }
                        }
                        return status;
                    });
    }
};

};  // namespace dds
};  // namespace qlib