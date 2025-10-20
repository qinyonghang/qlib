#include <gtest/gtest.h>
#include <iostream>

#include "qlib/yaml.h"

namespace qlib {

constexpr auto kText = R"(
# this is a test yaml file
log:
  level: 0 # 0: trace, 1: debug, 2: info, 3: warn, 4: error, 5: fatal

  console:
    enable: true
    level: 0

  file:
    enable: true
    level: 0
    path: auto

audio:
  reader:
    publish: "/audio/reader"

kws:
  subscriber: "/audio/reader"
  publisher: "/kws/wakeup"
  sample_rate: 16000
  model:
    transducer:
      encoder: "encoder-epoch-12-avg-2-chunk-16-left-64.int8.onnx"
      decoder: "decoder-epoch-12-avg-2-chunk-16-left-64.onnx"
      joiner: "joiner-epoch-12-avg-2-chunk-16-left-64.int8.onnx"
    tokens: "tokens.txt"
    debug: false
  keywords:
    path: "keywords.txt"

nlu:
)";

constexpr auto kBegin = kText;
constexpr auto kEnd = kBegin + len(kBegin);

TEST(qlib, YamlView) {
    yaml_view_t root{};
    auto result = yaml::parse(&root, kBegin, kEnd);
    EXPECT_EQ(result, 0);
    auto& log = root["log"];
    EXPECT_EQ(log["level"].get<int32_t>(), 0);
    auto& console = log["console"];
    EXPECT_EQ(console["enable"].get<bool_t>(), True);
    EXPECT_EQ(console["level"].get<int32_t>(), 0);
    auto& file = log["file"];
    EXPECT_EQ(file["enable"].get<bool_t>(), True);
    EXPECT_EQ(file["level"].get<int32_t>(), 0);
    EXPECT_EQ(file["path"].get<string_view_t>(), "auto");
    auto& audio = root["audio"];
    auto& reader = audio["reader"];
    EXPECT_EQ(reader["publish"].get<string_view_t>(), "/audio/reader");
    auto& kws = root["kws"];
    EXPECT_EQ(kws["subscriber"].get<string_view_t>(), "/audio/reader");
    EXPECT_EQ(kws["publisher"].get<string_view_t>(), "/kws/wakeup");
    EXPECT_EQ(kws["sample_rate"].get<int32_t>(), 16000);
    auto& model = kws["model"];
    auto& transducer = model["transducer"];
    EXPECT_EQ(transducer["encoder"].get<string_view_t>(),
              "encoder-epoch-12-avg-2-chunk-16-left-64.int8.onnx");
    EXPECT_EQ(transducer["decoder"].get<string_view_t>(),
              "decoder-epoch-12-avg-2-chunk-16-left-64.onnx");
    EXPECT_EQ(transducer["joiner"].get<string_view_t>(),
              "joiner-epoch-12-avg-2-chunk-16-left-64.int8.onnx");
    EXPECT_EQ(model["tokens"].get<string_view_t>(), "tokens.txt");
    EXPECT_EQ(model["debug"].get<bool_t>(), False);
    auto& keywords = kws["keywords"];
    EXPECT_EQ(keywords["path"].get<string_view_t>(), "keywords.txt");
    auto& nlu = root["nlu"];
    EXPECT_EQ(nlu.type(), yaml::null);
};

TEST(qlib, YamlCopy) {
    yaml_t root{};
    auto result = yaml::parse(&root, kBegin, kEnd);
    EXPECT_EQ(result, 0);
    auto& log = root["log"];
    EXPECT_EQ(log["level"].get<int32_t>(), 0);
    auto& console = log["console"];
    EXPECT_EQ(console["enable"].get<bool_t>(), True);
    EXPECT_EQ(console["level"].get<int32_t>(), 0);
    auto& file = log["file"];
    EXPECT_EQ(file["enable"].get<bool_t>(), True);
    EXPECT_EQ(file["level"].get<int32_t>(), 0);
    EXPECT_EQ(file["path"].get<string_view_t>(), "auto");
    auto& audio = root["audio"];
    auto& reader = audio["reader"];
    EXPECT_EQ(reader["publish"].get<string_view_t>(), "/audio/reader");
    auto& kws = root["kws"];
    EXPECT_EQ(kws["subscriber"].get<string_view_t>(), "/audio/reader");
    EXPECT_EQ(kws["publisher"].get<string_view_t>(), "/kws/wakeup");
    EXPECT_EQ(kws["sample_rate"].get<int32_t>(), 16000);
    auto& model = kws["model"];
    auto& transducer = model["transducer"];
    EXPECT_EQ(transducer["encoder"].get<string_view_t>(),
              "encoder-epoch-12-avg-2-chunk-16-left-64.int8.onnx");
    EXPECT_EQ(transducer["decoder"].get<string_view_t>(),
              "decoder-epoch-12-avg-2-chunk-16-left-64.onnx");
    EXPECT_EQ(transducer["joiner"].get<string_view_t>(),
              "joiner-epoch-12-avg-2-chunk-16-left-64.int8.onnx");
    EXPECT_EQ(model["tokens"].get<string_view_t>(), "tokens.txt");
    EXPECT_EQ(model["debug"].get<bool_t>(), False);
    auto& keywords = kws["keywords"];
    EXPECT_EQ(keywords["path"].get<string_view_t>(), "keywords.txt");
    auto& nlu = root["nlu"];
    EXPECT_EQ(nlu.type(), yaml::null);
};

};  // namespace qlib

int32_t main(int32_t argc, char* argv[]) {
    int32_t result{0};

    do {
        testing::InitGoogleTest(&argc, argv);
        result = RUN_ALL_TESTS();
    } while (false);

    return result;
}
