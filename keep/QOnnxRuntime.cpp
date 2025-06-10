// #include "QOnnxRuntime.h"

// #ifdef WITH_ONNXRUNTIME

// #include <filesystem>
// #include <fstream>
// #include <type_traits>
// #include <typeinfo>

// #include "QException.h"
// #include "QLog.h"

// namespace qlib {
// namespace {
// void dump_values(std::string const& path, Ort::Value const& value) {
//     auto mem_size = value.GetTensorTypeAndShapeInfo().GetElementCount();

//     switch (value.GetTensorTypeAndShapeInfo().GetElementType()) {
//         case ONNXTensorElementDataType::ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
//         case ONNXTensorElementDataType::ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
//         case ONNXTensorElementDataType::ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32:
//             mem_size *= 4u;
//             break;
//         case ONNXTensorElementDataType::ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16:
//         case ONNXTensorElementDataType::ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16:
//             mem_size *= 2u;
//             break;
//         case ONNXTensorElementDataType::ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8:
//         case ONNXTensorElementDataType::ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8:
//             mem_size *= 1u;
//             break;
//         default:;
//     }

//     std::ofstream out_f{path};
//     if (out_f.is_open()) {
//         out_f.write(value.GetTensorData<char>(), mem_size);
//     }
// }
// };  // namespace

// template <>
// int32_t QOnnxRuntime<false>::init(std::string const& model_path) {
//     int32_t result{0};

//     do {
//         auto impl = std::make_shared<QOnnxRuntimeImpl>();

//         impl->session = Ort::Session{impl->env, model_path.c_str(), impl->session_options};
//         Ort::AllocatorWithDefaultOptions allocator;
//         impl->cinput_names.resize(impl->session.GetInputCount());
//         for (auto i = 0u; i < impl->cinput_names.size(); ++i) {
//             impl->cinput_names[i] = impl->session.GetInputNameAllocated(i, allocator).get();
//         }
//         impl->coutput_names.resize(impl->session.GetOutputCount());
//         for (auto i = 0u; i < impl->coutput_names.size(); ++i) {
//             impl->coutput_names[i] = impl->session.GetOutputNameAllocated(i, allocator).get();
//         }

// #ifdef DUMP_TENSORS
//         if (!std::filesystem::exists(impl->dump_path)) {
//             std::filesystem::create_directories(impl->dump_path);
//         }
// #endif

//         __impl = impl;
//     } while (0);

//     return result;
// }

// template <>
// int32_t QOnnxRuntime<false>::run(QOnnxRuntimeOutputs* outputs, QOnnxRuntimeInputs const& inputs) {
//     int32_t result{0};

//     do {
//         auto impl = dynamic_cast<QOnnxRuntimeImpl*>(__impl.get());
//         if (impl == nullptr) {
//             qCMError("impl is nullptr");
//             break;
//         }

// #ifdef DUMP_TENSORS
//         for (auto i = 0u; i < impl->cinput_names.size(); ++i) {
//             auto path = fmt::format("{}/{}_input_{}_{}.npy", impl->dump_path, impl->idx, i,
//                                     impl->cinput_names[i]);
//             dump_values(path, inputs.at(i));
//         }
// #endif

//         auto output = impl->session.Run(Ort::RunOptions{nullptr}, impl->cinput_names.data(),
//                                         inputs.data(), inputs.size(), impl->coutput_names.data(),
//                                         impl->coutput_names.size());

// #ifdef DUMP_TENSORS
//         for (auto i = 0u; i < impl->coutput_names.size(); ++i) {
//             auto path = fmt::format("{}/{}_output_{}_{}.npy", impl->dump_path, impl->idx, i,
//                                     impl->coutput_names[i]);
//             dump_values(path, output.at(i));
//         }

//         ++impl->idx;
// #endif

//         *outputs = std::move(output);
//     } while (0);

//     return result;
// }

// template <>
// std::vector<std::string> const& QOnnxRuntime<false>::input_names() const {
//     auto impl = dynamic_cast<QOnnxRuntimeImpl*>(__impl.get());
//     if (impl == nullptr) {
//         QCMTHROW_EXCEPTION("impl is nullptr");
//     }

//     if (impl->input_names == nullptr) {
//         Ort::AllocatorWithDefaultOptions allocator;

//         impl->input_names =
//             std::make_shared<std::vector<std::string>>(impl->session.GetInputCount());
//         for (auto i = 0u; i < impl->input_names->size(); ++i) {
//             impl->input_names->at(i) = impl->session.GetInputNameAllocated(i, allocator).get();
//         }
//     }

//     return *impl->input_names;
// }

// template <>
// std::vector<std::string> const& QOnnxRuntime<false>::output_names() const {
//     auto impl = dynamic_cast<QOnnxRuntimeImpl*>(__impl.get());
//     if (impl == nullptr) {
//         QCMTHROW_EXCEPTION("impl is nullptr");
//     }

//     if (impl->output_names == nullptr) {
//         Ort::AllocatorWithDefaultOptions allocator;

//         impl->output_names =
//             std::make_shared<std::vector<std::string>>(impl->session.GetOutputCount());
//         for (auto i = 0u; i < impl->output_names->size(); ++i) {
//             impl->output_names->at(i) = impl->session.GetOutputNameAllocated(i, allocator).get();
//         }
//     }

//     return *impl->output_names;
// }

// template <>
// QOnnxRuntimeInputs& QOnnxRuntime<false>::inputs() {
//     auto impl = dynamic_cast<QOnnxRuntimeImpl*>(__impl.get());
//     if (impl == nullptr) {
//         QCMTHROW_EXCEPTION("impl is nullptr");
//     }

//     if (impl->inputs == nullptr) {
//         Ort::AllocatorWithDefaultOptions allocator;

//         impl->inputs = std::make_shared<QOnnxRuntimeInputs>();
//         for (auto i = 0u; i < impl->session.GetInputCount(); ++i) {
//             auto info = impl->session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
//             auto shape = info.GetShape();
//             auto ele_type = info.GetElementType();
//             impl->inputs->emplace_back(
//                 Ort::Value::CreateTensor(allocator, shape.data(), shape.size(), ele_type));
//         }
//     }

//     return *impl->inputs;
// }

// template <>
// QOnnxRuntimeOutputs& QOnnxRuntime<false>::outputs() {
//     auto impl = dynamic_cast<QOnnxRuntimeImpl*>(__impl.get());
//     if (impl == nullptr) {
//         QCMTHROW_EXCEPTION("impl is nullptr");
//     }

//     if (impl->outputs == nullptr) {
//         Ort::AllocatorWithDefaultOptions allocator;

//         impl->outputs = std::make_shared<QOnnxRuntimeInputs>();
//         for (auto i = 0u; i < impl->session.GetOutputCount(); ++i) {
//             auto info = impl->session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
//             auto shape = info.GetShape();
//             auto ele_type = info.GetElementType();
//             impl->outputs->emplace_back(
//                 Ort::Value::CreateTensor(allocator, shape.data(), shape.size(), ele_type));
//         }
//     }

//     return *impl->outputs;
// }

// };  // namespace qlib
// #endif
