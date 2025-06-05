#include <signal.h>

#include <atomic>
#include <filesystem>
#include <fstream>

#include "qlib/argparse.h"
#include "qlib/logger.h"
#include "qlib/rtsp.h"

namespace qlib {
class Application : public object {
public:
    using base = object;
    using self = Application;
    using ptr = std::shared_ptr<self>;

    template <class... Args>
    static ptr make(Args&&... args) {
        return std::make_shared<self>(std::forward<Args>(args)...);
    }

    template <class... Args>
    Application(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        if (0 != result) {
            std::exit(result);
        }
    }

    int32_t init(int32_t argc, char* argv[]) {
        int32_t result{0};

        do {
            signal(SIGINT, +[](int32_t) { self::exit = true; });

            argparse::ArgumentParser program(self::name);

            program.add_argument("url").default_value<string_t>("rtsp://localhost:554/stream");
            program.add_argument("file").default_value<std::filesystem::path>("stream.h264");
            program.add_argument("--width").default_value<uint32_t>(1920u);
            program.add_argument("--height").default_value<uint32_t>(1080u);
            program.add_argument("--fps").default_value<uint32_t>(30u);
            program.add_argument("--bitrate").default_value<uint32_t>(16000000u);
            program.add_argument("--loop").default_value<bool>(true);

            try {
                program.parse_args(argc, argv);
            } catch (std::exception const& err) {
                std::cout << err.what() << std::endl;
                std::cout << program;
                result = -1;
                break;
            }

            auto url = program.get<string_t>("url");
            auto file = program.get<std::filesystem::path>("file");
            auto width = program.get<uint32_t>("width");
            auto height = program.get<uint32_t>("height");
            auto fps = program.get<uint32_t>("fps");
            auto bitrate = program.get<uint32_t>("bitrate");
            auto loop = program.get<bool>("loop");

            if (!std::filesystem::exists(file)) {
                qError("File({}) do not exists!", file);
                result = -1;
                break;
            }

            qInfo("url: {}, width: {}, height: {}, fps: {}, bitrate: {}", url, width, height, fps,
                  bitrate);
            writer_ptr = rtsp::writer::make(url, width, height, fps, bitrate);
            if (nullptr == writer_ptr) {
                qError("Create RTSP Writer Failed! Please check your url!");
                result = -1;
                break;
            }

        } while (false);

        return result;
    }

    int32_t exec() {
        while (!self::exit) {
            auto filesize = std::filesystem::file_size(self::file);
            if (filesize <= 0) {
                qError("File size is zero!");
                break;
            }

            std::ifstream ifs{self::file};
            if (!ifs.is_open()) {
                qError("Open File Failed!");
                break;
            }

            std::vector<uint8_t> buf(filesize);
            ifs.read(reinterpret_cast<char*>(buf.data()), filesize);
            if (ifs.bad()) {
                qError("Read File Failed!");
                break;
            }

            int32_t start = -1;
            int32_t end = -1;
            for (auto i = 0u; i + 3u < buf.size() && !self::exit; ++i) {
                if (buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0x00 &&
                    buf[i + 3] == 0x01) {
                    end = i;
                }

                if (likely((start != -1 && start != end))) {
                    self::writer_ptr->emplace(
                        rtsp::writer::frame(buf.data() + start, buf.data() + end));
                }

                start = end;
            }

            if (!self::loop) {
                break;
            }

            qTrace("Running...");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return 0;
    }

protected:
    constexpr static inline auto name = "dds";
    static inline std::atomic_bool exit{false};

    std::filesystem::path file;
    bool loop{true};
    rtsp::writer::ptr writer_ptr{nullptr};
};
};  // namespace qlib

int32_t main(int32_t argc, char* argv[]) {
    auto app = qlib::Application::make(argc, argv);
    return app->exec();
}
