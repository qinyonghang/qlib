#pragma once

#include <atomic>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "QException.h"
#include "QLog.h"
#include "QObject.h"
#include "QTimeProfile.h"

namespace qlib {
template <typename QDerived>
class QModelRuntime : public QObject {
public:
    using QType = QModelRuntime<QDerived>;
    using QIAttr = typename QTraits<QDerived>::QIAttr;
    using QOAttr = typename QTraits<QDerived>::QOAttr;
    using QIType = typename QTraits<QDerived>::QIType;
    using QOType = typename QTraits<QDerived>::QOType;

public:
    using QObject::QObject;

    virtual int32_t init(std::string const&) = 0;
    virtual int32_t run(QOType*, QIType const&) = 0;

    virtual QIAttr const& input_attrs() const = 0;
    virtual QOAttr const& output_attrs() const = 0;

    virtual QIType& inputs() = 0;
    virtual QOType& outputs() = 0;

    virtual QIType const& inputs() const { return const_cast<decltype(*this)>(*this).inputs(); }
    virtual QOType const& outputs() const { return const_cast<decltype(*this)>(*this).outputs(); }
};

template <typename QInputs, typename QOutputs, typename QModelRuntime, bool async = false>
class QModel : public QObject {
public:
    using QType = QModel<QInputs, QOutputs, QModelRuntime, async>;
    using QIType = QInputs;
    using QOType = QOutputs;
    using QRType = QModelRuntime;
    using QRIType = typename QModelRuntime::QIType;
    using QROType = typename QModelRuntime::QOType;

protected:
    struct QModelImpl : public QObject {
        std::shared_ptr<QModelRuntime> runtime;
    };

public:
    QModel(QObject* parent = nullptr) : QObject(parent) {}

    QModel(std::string const& model_path, QObject* parent = nullptr) : QObject(parent) {
        int32_t result{init(model_path)};
        if (result != 0) {
            QTHROW_EXCEPTION("QModel::init return {}", result);
        }
    }

    virtual int32_t init(std::string const& model_path) {
        int32_t result{0};

        do {
            auto impl = std::make_shared<QModelImpl>();

            impl->runtime = std::make_shared<QModelRuntime>(model_path, this);

            __impl = impl;
        } while (0);

        return result;
    }

    virtual int32_t run(QOutputs* outputs, QInputs const& inputs) {
        int32_t result{0};

        do {
            auto impl = dynamic_cast<QModelImpl*>(__impl.get());
            if (impl == nullptr) {
                result = -1;
                qCMError("impl is nullptr");
                break;
            }

            auto& _inputs = impl->runtime->inputs();
            auto& _outputs = impl->runtime->outputs();

            {
                QTimeProfile profile("QModel::preprocess");
                result = this->preprocess(&_inputs, inputs);
                if (result != 0) {
                    break;
                }
            }

            {
                QTimeProfile profile("QModel::runtime::run");
                result = impl->runtime->run(&_outputs, _inputs);
                if (result != 0) {
                    break;
                }
            }

            {
                QTimeProfile profile("QModel::postprocess");
                result = this->postprocess(outputs, _outputs);
                if (result != 0) {
                    break;
                }
            }
        } while (0);

        return result;
    }

protected:
    QObjectPtr __impl;

    virtual int32_t preprocess(QRIType*, QInputs const&) = 0;
    virtual int32_t postprocess(QOutputs*, QROType const&) = 0;
};

template <typename QInputs, typename QOutputs, typename QModelRuntime>
class QModel<QInputs, QOutputs, QModelRuntime, true> : public QObject {
public:
    using QType = QModel<QInputs, QOutputs, QModelRuntime, true>;
    using QIType = QInputs;
    using QOType = QOutputs;
    using QRType = QModelRuntime;
    using QRIType = typename QModelRuntime::QIType;
    using QROType = typename QModelRuntime::QOType;

protected:
    struct QModelImpl : public QObject {
        std::shared_ptr<QModelRuntime> runtime;

        std::atomic<size_t> id{0u};
        std::shared_ptr<std::queue<std::tuple<size_t, QInputs>>> inputs_p;
        std::shared_ptr<std::queue<std::tuple<size_t, QOutputs>>> outputs_p;
        std::shared_ptr<std::queue<std::tuple<size_t, QRIType>>> rinputs_q;
        std::shared_ptr<std::queue<std::tuple<size_t, QROType>>> routputs_q;
        int32_t queue_size{-1};

        std::thread preprocess_t;
        std::thread postprocess_t;
        std::thread infer_t;
        std::atomic<bool> exit{false};
    };

public:
    QModel(QObject* parent = nullptr) : QObject(parent) {}

    QModel(std::string const& model_path, QObject* parent = nullptr) : QObject(parent) {
        int32_t result{init(model_path)};
        if (result != 0) {
            QCMTHROW_EXCEPTION("init return {}", result);
        }
    }

    QModel(std::string const& model_path, int32_t queue_size, QObject* parent = nullptr)
            : QObject(parent) {
        int32_t result{init(model_path, queue_size)};
        if (result != 0) {
            QCMTHROW_EXCEPTION("init return {}", result);
        }
    }

    ~QModel() override {
        auto impl = dynamic_cast<QModelImpl*>(__impl.get());

        if (impl != nullptr) {
            impl->exit = true;
            impl->preprocess_t.join();
            impl->postprocess_t.join();
            impl->infer_t.join();
        }
    }

    int32_t init(std::string const& model_path, int32_t queue_size = -1) {
        int32_t result{0};

        do {
            auto impl = std::make_shared<QModelImpl>();

            impl->runtime = std::make_shared<QModelRuntime>(model_path, this);
            impl->id = 0u;
            impl->inputs_q = std::make_shared<std::queue<decltype(*impl->inputs_q)>>();
            impl->outputs_q = std::make_shared<std::queue<decltype(*impl->outputs_q)>>();
            impl->queue_size = queue_size;

            impl->preprocess_t = std::thread([this]() {
                auto impl = dynamic_cast<QModelImpl*>(__impl.get());
                while (!impl->exit.load()) {
                    auto& [id, inputs] = impl->inputs_q->front();
                    impl->inputs_q->pop();
                    QRIType rinputs;

                    auto result = this->preprocess(&rinputs, inputs);
                    if (result != 0) {
                        qCMError("preprocess return {}...", result);
                        continue;
                    }

                    impl->rinputs_q->emplace(id, rinputs);
                }
            });

            impl->postprocess_t = std::thread([this]() {
                auto impl = dynamic_cast<QModelImpl*>(__impl.get());
                while (!impl->exit.load()) {
                    auto& [id, routputs] = impl->outputs_q->front();
                    impl->outputs_q->pop();
                    QOutputs outputs;

                    auto result = impl->postprocess(&outputs, routputs);
                    if (result != 0) {
                        qCMError("preprocess return {}...", result);
                        continue;
                    }

                    impl->outputs_p->emplace(id, outputs);
                }
            });

            impl->infer_t = std::thread([this]() {
                auto impl = dynamic_cast<QModelImpl*>(__impl.get());
                while (!impl->exit.load()) {
                    auto& [id, rinputs] = impl->rinputs_q->front();
                    impl->rinputs_q->pop();
                    QROType routputs;

                    auto result = impl->runtime->run(&routputs, rinputs);
                    if (result != 0) {
                        qCMError("runtime::run return {}...", result);
                        continue;
                    }

                    impl->outputs_q->emplace(id, routputs);
                }
            });

            __impl = impl;
        } while (0);

        return result;
    }

    template <typename... Args>
    auto push(Args&&... inputs) {
        int32_t result{0};
        size_t id{0u};

        do {
            auto impl = dynamic_cast<QModelImpl*>(__impl.get());
            if (impl == nullptr) {
                result = -1;
                break;
            }

            if (impl->queue_size > 0 &&
                impl->inputs_q->size() >= static_cast<size_t>(impl->queue_size)) {
                result = -1;
                qError("QModel::push(): queue full! queue_size: {}", impl->queue_size);
                break;
            }

            id = impl->id.load();
            impl->inputs_q->emplace(id, std::forward<Args>(inputs)...);
            ++impl->id;
        } while (0);

        return std::make_pair(result, id);
    }

    auto pop(QOutputs* outputs) {
        int32_t result{0};
        size_t id{0u};

        do {
            auto impl = dynamic_cast<QModelImpl*>(__impl.get());
            if (impl == nullptr) {
                result = -1;
                break;
            }

            if (impl->outputs_q->empty()) {
                result = -1;
                qError("QModel::pop(): queue empty! outputs_p: {}", static_cast<void*>(outputs));
                break;
            }

            std::tie(id, *outputs) = impl->outputs_q->front();
            impl->outputs_q->pop();
        } while (0);

        return std::make_pair(result, id);
    }

protected:
    QObjectPtr __impl;

    virtual int32_t preprocess(QRIType*, QInputs const&) = 0;
    virtual int32_t postprocess(QOutputs*, QROType const&) = 0;
};

};  // namespace qlib