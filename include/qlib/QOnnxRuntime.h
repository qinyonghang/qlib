// #pragma once

// #ifdef WITH_ONNXRUNTIME

// #include "QModel.h"
// #include "onnxruntime_cxx_api.h"

// namespace qlib {
// using QOnnxRuntimeInputs = std::vector<Ort::Value>;
// using QOnnxRuntimeOutputs = std::vector<Ort::Value>;

// template <bool with_gpu = false>
// class QOnnxRuntime : public QModelRuntime<QOnnxRuntimeInputs, QOnnxRuntimeOutputs> {
// protected:
//     struct QOnnxRuntimeImpl : public QObject {
//         Ort::Env env{nullptr};
//         Ort::SessionOptions session_options{nullptr};
//         Ort::Session session{nullptr};
//         std::vector<char const*> cinput_names;
//         std::vector<char const*> coutput_names;
//         std::shared_ptr<std::vector<std::string>> input_names{nullptr};
//         std::shared_ptr<std::vector<std::string>> output_names{nullptr};
//         std::shared_ptr<QOnnxRuntimeInputs> inputs{nullptr};
//         std::shared_ptr<QOnnxRuntimeOutputs> outputs{nullptr};

// #ifdef DUMP_TENSORS
//         size_t idx{0u};
//         std::string dump_path{"tmp"};
// #endif
//     };

// public:
//     using QModelRuntime::QModelRuntime;

//     QOnnxRuntime(std::string const& model_path, QObject* parent = nullptr)
//             : QModelRuntime<QOnnxRuntimeInputs, QOnnxRuntimeOutputs>(parent) {
//         int32_t result{init(model_path)};
//         if (result != 0) {
//             QTHROW_EXCEPTION("QModelRuntime::init return {}", result);
//         }
//     }

//     int32_t init(std::string const& model_path) override;
//     int32_t run(QOnnxRuntimeOutputs* outputs, QOnnxRuntimeInputs const& inputs) override;

//     std::vector<std::string> const& input_names() const override;
//     std::vector<std::string> const& output_names() const override;

//     QOnnxRuntimeInputs& inputs() override;
//     QOnnxRuntimeOutputs& outputs() override;

// protected:
//     QObjectPtr __impl;
// };

// template <>
// class QOnnxRuntime<true> : public QOnnxRuntime<false> {
// public:
//     explicit QOnnxRuntime(QObject* parent = nullptr) : QOnnxRuntime<false>{parent} {}

//     QOnnxRuntime(std::string const& model_path, size_t device_id = 0u, QObject* parent = nullptr)
//             : QOnnxRuntime<false>{parent} {
//         int32_t result{init(model_path, device_id)};
//         if (result != 0) {
//             QCMTHROW_EXCEPTION("init return {}... model_path = {}, device_id = {}", result,
//                                model_path, device_id);
//         }
//     }

//     int32_t init(std::string const& model_path, size_t device_id = 0u) {
//         int32_t result{0};

//         do {
//             auto impl = std::make_shared<QOnnxRuntimeImpl>();

//             OrtCUDAProviderOptions options;
//             options.device_id = device_id;

//             impl->session_options.AppendExecutionProvider_CUDA(options);
//             impl->session = Ort::Session{impl->env, model_path.c_str(), impl->session_options};
//             Ort::AllocatorWithDefaultOptions allocator;
//             impl->cinput_names.resize(impl->session.GetInputCount());
//             for (auto i = 0u; i < impl->cinput_names.size(); ++i) {
//                 impl->cinput_names[i] = impl->session.GetInputNameAllocated(i, allocator).get();
//             }
//             impl->coutput_names.resize(impl->session.GetOutputCount());
//             for (auto i = 0u; i < impl->coutput_names.size(); ++i) {
//                 impl->coutput_names[i] = impl->session.GetOutputNameAllocated(i, allocator).get();
//             }

//             __impl = impl;
//         } while (0);

//         return result;
//     }
// };

// };  // namespace qlib

// #endif
