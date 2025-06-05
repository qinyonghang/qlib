#include "qlib/psdk.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <libusb-1.0/libusb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>

#include "dji_aircraft_info.h"
#include "dji_core.h"
#include "dji_fc_subscription.h"
#include "dji_flight_controller.h"
#include "dji_liveview.h"
#include "dji_logger.h"
#include "dji_perception.h"
#include "dji_platform.h"
#include "dji_waypoint_v2.h"
#include "qlib/logger.h"
#include "qlib/singleton.h"

namespace qlib {
namespace psdk {

namespace {
constexpr auto SOCKET_RECV_BUF_MAX_SIZE = (1000 * 1000 * 10);

typedef struct {
    int socketFd;
} T_SocketHandleStruct;

typedef struct {
    int32_t uartFd;
} T_UartHandleStruct;

typedef struct {
    libusb_device_handle* handle;
    int32_t ep1;
    int32_t ep2;
    uint32_t interfaceNum;
    T_DjiHalUsbBulkInfo usbBulkInfo;
} T_HalUsbBulkObj;

class register2 final : public object {
public:
    using self = register2;
    using ptr = std::shared_ptr<self>;

    register2(init_parameter const& parameter) {
        int32_t result{init(parameter)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
        qTrace("PSDK Init!");
    }

    ~register2() {
        T_DjiReturnCode result{DjiCore_DeInit()};
        if (result != 0) {
            qWarn("DjiCore_DeInit return {:#x}!", result);
        } else {
            qTrace("PSDK Deinit!");
        }
    }

    int32_t init(init_parameter const& parameter) {
        int32_t result{0};

        do {
            static init_parameter PARAMETER = parameter;

            T_DjiOsalHandler osalHandler = {
                .TaskCreate = +[](const char* name, void* (*taskFunc)(void*), uint32_t stackSize,
                                  void* arg, T_DjiTaskHandle* task) -> T_DjiReturnCode {
                    int result;
                    char nameDealed[16] = {0};

                    *task = (pthread_t*)malloc(sizeof(pthread_t));
                    if (*task == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
                    }

                    result = pthread_create((pthread_t*)(*task), NULL, taskFunc, arg);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    if (name != NULL)
                        strncpy(nameDealed, name, sizeof(nameDealed) - 1);
                    result = pthread_setname_np(*(pthread_t*)*task, nameDealed);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
                    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .TaskDestroy = +[](T_DjiTaskHandle task) -> T_DjiReturnCode {
                    if (task == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }
                    pthread_cancel(*(pthread_t*)task);
                    free(task);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .TaskSleepMs = +[](uint32_t timeMs) -> T_DjiReturnCode {
                    usleep(1000 * timeMs);
                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .MutexCreate = +[](T_DjiMutexHandle* mutex_ptr) -> T_DjiReturnCode {
                    int result;

                    if (!mutex_ptr) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    *mutex_ptr = malloc(sizeof(pthread_mutex_t));
                    if (*mutex_ptr == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
                    }

                    result =
                        pthread_mutex_init(reinterpret_cast<pthread_mutex_t*>(*mutex_ptr), NULL);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .MutexDestroy = +[](T_DjiMutexHandle mutex) -> T_DjiReturnCode {
                    int result = 0;

                    if (!mutex) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    result = pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(mutex));
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }
                    free(mutex);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .MutexLock = +[](T_DjiMutexHandle mutex) -> T_DjiReturnCode {
                    int result = 0;

                    if (!mutex) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    result = pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mutex));
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .MutexUnlock = +[](T_DjiMutexHandle mutex) -> T_DjiReturnCode {
                    int result = 0;

                    if (!mutex) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    result = pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mutex));
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .SemaphoreCreate =
                    +[](uint32_t initValue, T_DjiSemaHandle* semaphore) -> T_DjiReturnCode {
                    int result;

                    *semaphore = malloc(sizeof(sem_t));
                    if (*semaphore == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
                    }

                    result = sem_init((sem_t*)*semaphore, 0, (unsigned int)initValue);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .SemaphoreDestroy = +[](T_DjiSemaHandle semaphore) -> T_DjiReturnCode {
                    int result;

                    result = sem_destroy((sem_t*)semaphore);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    free(semaphore);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .SemaphoreWait = +[](T_DjiSemaHandle semaphore) -> T_DjiReturnCode {
                    int result;

                    result = sem_wait((sem_t*)semaphore);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .SemaphoreTimedWait =
                    +[](T_DjiSemaHandle semaphore, uint32_t waitTime) -> T_DjiReturnCode {
                    int result;
                    struct timespec semaphoreWaitTime;
                    struct timeval systemTime;

                    gettimeofday(&systemTime, NULL);

                    systemTime.tv_usec += waitTime * 1000;
                    if (systemTime.tv_usec >= 1000000) {
                        systemTime.tv_sec += systemTime.tv_usec / 1000000;
                        systemTime.tv_usec %= 1000000;
                    }

                    semaphoreWaitTime.tv_sec = systemTime.tv_sec;
                    semaphoreWaitTime.tv_nsec = systemTime.tv_usec * 1000;

                    result = sem_timedwait((sem_t*)semaphore, &semaphoreWaitTime);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .SemaphorePost = +[](T_DjiSemaHandle semaphore) -> T_DjiReturnCode {
                    int result;

                    result = sem_post((sem_t*)semaphore);
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .GetTimeMs = +[](uint32_t* ms) -> T_DjiReturnCode {
                    struct timeval time;

                    gettimeofday(&time, NULL);
                    *ms = (time.tv_sec * 1000 + time.tv_usec / 1000);

                    static uint32_t s_localTimeMsOffset = 0;
                    if (s_localTimeMsOffset == 0) {
                        s_localTimeMsOffset = *ms;
                    } else {
                        *ms = *ms - s_localTimeMsOffset;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .GetTimeUs = +[](uint64_t* us) -> T_DjiReturnCode {
                    struct timeval time;

                    gettimeofday(&time, NULL);
                    *us = (time.tv_sec * 1000000 + time.tv_usec);
                    static uint64_t s_localTimeUsOffset = 0;
                    if (s_localTimeUsOffset == 0) {
                        s_localTimeUsOffset = *us;
                    } else {
                        *us = *us - s_localTimeUsOffset;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .GetRandomNum = +[](uint16_t* randomNum) -> T_DjiReturnCode {
                    srand(time(NULL));
                    *randomNum = random() % 65535;

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .Malloc = +[](uint32_t size) { return malloc(size); },
                .Free = +[](void* ptr) { free(ptr); },
            };

            T_DjiLoggerConsole printConsole = {
                .func = +[](uint8_t const* data, uint16_t dataLen) -> T_DjiReturnCode {
                    std::cout << "DJI: " << reinterpret_cast<char const*>(data);
                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,
                .isSupportColor = true,
            };

            T_DjiLoggerConsole printFile = {
                .func = +[](uint8_t const* data, uint16_t dataLen) -> T_DjiReturnCode {
                    static std::ofstream ofs{[]() -> std::string {
                        auto& parameter = PARAMETER;
                        auto now = std::chrono::system_clock::now();
                        auto time = std::chrono::system_clock::to_time_t(now);
                        auto filename = fmt::format("{}/DJI_{:%Y-%m-%d_%H-%M-%S}.log",
                                                    parameter.log_prefix, fmt::localtime(time));
                        std::filesystem::create_directories(parameter.log_prefix);
                        return filename;
                    }()};

                    ofs << data;

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_DEBUG,
                .isSupportColor = true,
            };

            T_DjiHalUartHandler uartHandler = {
                .UartInit = +[](E_DjiHalUartNum uartNum, uint32_t baudRate,
                                T_DjiUartHandle* uartHandle) -> T_DjiReturnCode {
                    auto& parameter = PARAMETER;
                    T_UartHandleStruct* uartHandleStruct = NULL;
                    struct termios options;
                    struct flock lock;
                    T_DjiReturnCode returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                    FILE* fp;

                    std::string uartName;
                    if (uartNum == DJI_HAL_UART_NUM_0) {
                        uartName = parameter.uart1;
                    } else if (uartNum == DJI_HAL_UART_NUM_1) {
                        uartName = parameter.uart2;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    uartHandleStruct = (T_UartHandleStruct*)malloc(sizeof(T_UartHandleStruct));
                    if (uartHandleStruct == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
                    }

                    auto systemCmd = fmt::format("chmod 777 {}", uartName);
                    fp = popen(systemCmd.c_str(), "r");
                    if (fp == NULL) {
                        goto free_uart_handle;
                    }

                    uartHandleStruct->uartFd =
                        open(uartName.c_str(),
                             (unsigned)O_RDWR | (unsigned)O_NOCTTY | (unsigned)O_NDELAY);
                    if (uartHandleStruct->uartFd == -1) {
                        goto close_fp;
                    }

                    // Forbid multiple psdk programs to access the serial port
                    lock.l_type = F_WRLCK;
                    lock.l_pid = getpid();
                    lock.l_whence = SEEK_SET;
                    lock.l_start = 0;
                    lock.l_len = 0;

                    if (fcntl(uartHandleStruct->uartFd, F_GETLK, &lock) < 0) {
                        goto close_uart_fd;
                    }
                    if (lock.l_type != F_UNLCK) {
                        goto close_uart_fd;
                    }
                    lock.l_type = F_WRLCK;
                    lock.l_pid = getpid();
                    lock.l_whence = SEEK_SET;
                    lock.l_start = 0;
                    lock.l_len = 0;
                    if (fcntl(uartHandleStruct->uartFd, F_SETLKW, &lock) < 0) {
                        goto close_uart_fd;
                    }

                    if (tcgetattr(uartHandleStruct->uartFd, &options) != 0) {
                        goto close_uart_fd;
                    }

                    switch (baudRate) {
                        case 115200:
                            cfsetispeed(&options, B115200);
                            cfsetospeed(&options, B115200);
                            break;
                        case 230400:
                            cfsetispeed(&options, B230400);
                            cfsetospeed(&options, B230400);
                            break;
                        case 460800:
                            cfsetispeed(&options, B460800);
                            cfsetospeed(&options, B460800);
                            break;
                        case 921600:
                            cfsetispeed(&options, B921600);
                            cfsetospeed(&options, B921600);
                            break;
                        case 1000000:
                            cfsetispeed(&options, B1000000);
                            cfsetospeed(&options, B1000000);
                            break;
                        default:
                            goto close_uart_fd;
                    }

                    options.c_cflag |= (unsigned)CLOCAL;
                    options.c_cflag |= (unsigned)CREAD;
                    options.c_cflag &= ~(unsigned)CRTSCTS;
                    options.c_cflag &= ~(unsigned)CSIZE;
                    options.c_cflag |= (unsigned)CS8;
                    options.c_cflag &= ~(unsigned)PARENB;
                    options.c_iflag &= ~(unsigned)INPCK;
                    options.c_cflag &= ~(unsigned)CSTOPB;
                    options.c_oflag &= ~(unsigned)OPOST;
                    options.c_lflag &=
                        ~((unsigned)ICANON | (unsigned)ECHO | (unsigned)ECHOE | (unsigned)ISIG);
                    options.c_iflag &= ~((unsigned)BRKINT | (unsigned)ICRNL | (unsigned)INPCK |
                                         (unsigned)ISTRIP | (unsigned)IXON);
                    options.c_cc[VTIME] = 0;
                    options.c_cc[VMIN] = 0;

                    tcflush(uartHandleStruct->uartFd, TCIFLUSH);

                    if (tcsetattr(uartHandleStruct->uartFd, TCSANOW, &options) != 0) {
                        goto close_uart_fd;
                    }

                    *uartHandle = uartHandleStruct;
                    pclose(fp);

                    return returnCode;

                close_uart_fd:
                    close(uartHandleStruct->uartFd);

                close_fp:
                    pclose(fp);

                free_uart_handle:
                    free(uartHandleStruct);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                },
                .UartDeInit = +[](T_DjiUartHandle uartHandle) -> T_DjiReturnCode {
                    int32_t ret;
                    T_UartHandleStruct* uartHandleStruct = (T_UartHandleStruct*)uartHandle;

                    if (uartHandle == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
                    }

                    ret = close(uartHandleStruct->uartFd);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    free(uartHandleStruct);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UartWriteData = +[](T_DjiUartHandle uartHandle, const uint8_t* buf, uint32_t len,
                                     uint32_t* realLen) -> T_DjiReturnCode {
                    int32_t ret;
                    T_UartHandleStruct* uartHandleStruct = (T_UartHandleStruct*)uartHandle;

                    if (uartHandle == NULL || buf == NULL || len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = write(uartHandleStruct->uartFd, buf, len);
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UartReadData = +[](T_DjiUartHandle uartHandle, uint8_t* buf, uint32_t len,
                                    uint32_t* realLen) -> T_DjiReturnCode {
                    int32_t ret;
                    T_UartHandleStruct* uartHandleStruct = (T_UartHandleStruct*)uartHandle;

                    if (uartHandle == NULL || buf == NULL || len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = read(uartHandleStruct->uartFd, buf, len);
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UartGetStatus =
                    +[](E_DjiHalUartNum uartNum, T_DjiUartStatus* status) -> T_DjiReturnCode {
                    if (uartNum == DJI_HAL_UART_NUM_0) {
                        status->isConnect = true;
                    } else if (uartNum == DJI_HAL_UART_NUM_1) {
                        status->isConnect = true;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
            };

            T_DjiHalNetworkHandler networkHandler = {
                .NetworkInit = +[](const char* ipAddr, const char* netMask,
                                   T_DjiNetworkHandle* halObj) -> T_DjiReturnCode {
                    auto& parameter = PARAMETER;
                    int32_t ret;

                    if (ipAddr == NULL || netMask == NULL) {
                        USER_LOG_ERROR("hal network config param error");
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    //Attention: need root permission to config ip addr and netmask.
                    auto cmdStr = fmt::format("ifconfig {} up", parameter.network.name);
                    ret = system(cmdStr.c_str());
                    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                        USER_LOG_ERROR("Can't open the network."
                                       "Probably the program not execute with root permission."
                                       "Please use the root permission to execute the program.");
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    cmdStr = fmt::format("ifconfig {} {} netmask {}", parameter.network.name,
                                         ipAddr, netMask);
                    ret = system(cmdStr.c_str());
                    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                        USER_LOG_ERROR("Can't config the ip address of network."
                                       "Probably the program not execute with root permission."
                                       "Please use the root permission to execute the program.");
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .NetworkDeInit = +[](T_DjiNetworkHandle halObj) -> T_DjiReturnCode {
                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .NetworkGetDeviceInfo =
                    +[](T_DjiHalNetworkDeviceInfo* deviceInfo) -> T_DjiReturnCode {
                    auto& parameter = PARAMETER;
                    auto& network = parameter.network;
                    deviceInfo->usbNetAdapter.vid = network.vid;
                    deviceInfo->usbNetAdapter.pid = network.pid;

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
            };

            T_DjiHalUsbBulkHandler usbBulkHandler = {
                .UsbBulkInit = +[](T_DjiHalUsbBulkInfo usbBulkInfo,
                                   T_DjiUsbBulkHandle* usbBulkHandle) -> T_DjiReturnCode {
                    int32_t ret;
                    struct libusb_device_handle* handle = NULL;

                    *usbBulkHandle = malloc(sizeof(T_HalUsbBulkObj));
                    if (*usbBulkHandle == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    if (usbBulkInfo.isUsbHost == true) {
                        ret = libusb_init(NULL);
                        if (ret < 0) {
                            USER_LOG_ERROR("init usb bulk failed, errno = %d", ret);
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                        }

                        handle =
                            libusb_open_device_with_vid_pid(NULL, usbBulkInfo.vid, usbBulkInfo.pid);
                        if (handle == NULL) {
                            USER_LOG_ERROR("open usb device failed");
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                        }

                        ret = libusb_claim_interface(handle, usbBulkInfo.channelInfo.interfaceNum);
                        if (ret != LIBUSB_SUCCESS) {
                            USER_LOG_ERROR("libusb claim interface failed, errno = %d", ret);
                            libusb_close(handle);
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                        }

                        ((T_HalUsbBulkObj*)*usbBulkHandle)->handle = handle;
                        memcpy(&((T_HalUsbBulkObj*)*usbBulkHandle)->usbBulkInfo, &usbBulkInfo,
                               sizeof(usbBulkInfo));
                    } else {
                        ((T_HalUsbBulkObj*)*usbBulkHandle)->handle = handle;
                        memcpy(&((T_HalUsbBulkObj*)*usbBulkHandle)->usbBulkInfo, &usbBulkInfo,
                               sizeof(usbBulkInfo));
                        ((T_HalUsbBulkObj*)*usbBulkHandle)->interfaceNum =
                            usbBulkInfo.channelInfo.interfaceNum;

                        auto& parameter = PARAMETER;
                        auto& bulk1 = parameter.usb.bulk1;
                        auto& bulk2 = parameter.usb.bulk2;
                        if (usbBulkInfo.channelInfo.interfaceNum == bulk1.interface_num) {
                            ((T_HalUsbBulkObj*)*usbBulkHandle)->ep1 =
                                open(bulk1.ep_out.c_str(), O_RDWR);
                            if (((T_HalUsbBulkObj*)*usbBulkHandle)->ep1 < 0) {
                                return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                            }

                            ((T_HalUsbBulkObj*)*usbBulkHandle)->ep2 =
                                open(bulk1.ep_in.c_str(), O_RDWR);
                            if (((T_HalUsbBulkObj*)*usbBulkHandle)->ep2 < 0) {
                                return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                            }
                        } else if (usbBulkInfo.channelInfo.interfaceNum == bulk2.interface_num) {
                            ((T_HalUsbBulkObj*)*usbBulkHandle)->ep1 =
                                open(bulk2.ep_out.c_str(), O_RDWR);
                            if (((T_HalUsbBulkObj*)*usbBulkHandle)->ep1 < 0) {
                                return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                            }

                            ((T_HalUsbBulkObj*)*usbBulkHandle)->ep2 =
                                open(bulk2.ep_in.c_str(), O_RDWR);
                            if (((T_HalUsbBulkObj*)*usbBulkHandle)->ep2 < 0) {
                                return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                            }
                        }
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UsbBulkDeInit = +[](T_DjiUsbBulkHandle usbBulkHandle) -> T_DjiReturnCode {
                    struct libusb_device_handle* handle = NULL;
                    int32_t ret;
                    T_DjiOsalHandler* osalHandler = DjiPlatform_GetOsalHandler();

                    if (usbBulkHandle == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    handle = ((T_HalUsbBulkObj*)usbBulkHandle)->handle;

                    if (((T_HalUsbBulkObj*)usbBulkHandle)->usbBulkInfo.isUsbHost == true) {
                        ret = libusb_release_interface(handle,
                                                       ((T_HalUsbBulkObj*)usbBulkHandle)
                                                           ->usbBulkInfo.channelInfo.interfaceNum);
                        if (ret != 0) {
                            USER_LOG_ERROR("release usb bulk interface failed, errno = %d", ret);
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                        }
                        osalHandler->TaskSleepMs(100);
                        libusb_exit(NULL);
                    } else {
                        close(((T_HalUsbBulkObj*)usbBulkHandle)->ep1);
                        close(((T_HalUsbBulkObj*)usbBulkHandle)->ep2);
                    }

                    free(usbBulkHandle);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UsbBulkWriteData = +[](T_DjiUsbBulkHandle usbBulkHandle, const uint8_t* buf,
                                        uint32_t len, uint32_t* realLen) -> T_DjiReturnCode {
                    int32_t ret;
                    int32_t actualLen;
                    struct libusb_device_handle* handle = NULL;

                    if (usbBulkHandle == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    handle = ((T_HalUsbBulkObj*)usbBulkHandle)->handle;

                    if (((T_HalUsbBulkObj*)usbBulkHandle)->usbBulkInfo.isUsbHost == true) {
                        ret = libusb_bulk_transfer(
                            handle,
                            ((T_HalUsbBulkObj*)usbBulkHandle)->usbBulkInfo.channelInfo.endPointOut,
                            (uint8_t*)buf, len, &actualLen, 50);
                        if (ret < 0) {
                            USER_LOG_ERROR("Write usb bulk data failed, errno = %d", ret);
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                        }

                        *realLen = actualLen;
                    } else {
                        *realLen = write(((T_HalUsbBulkObj*)usbBulkHandle)->ep1, buf, len);
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UsbBulkReadData = +[](T_DjiUsbBulkHandle usbBulkHandle, uint8_t* buf, uint32_t len,
                                       uint32_t* realLen) -> T_DjiReturnCode {
                    int32_t ret;
                    struct libusb_device_handle* handle = NULL;
                    int32_t actualLen;

                    if (usbBulkHandle == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    handle = ((T_HalUsbBulkObj*)usbBulkHandle)->handle;

                    if (((T_HalUsbBulkObj*)usbBulkHandle)->usbBulkInfo.isUsbHost == true) {
                        ret = libusb_bulk_transfer(
                            handle,
                            ((T_HalUsbBulkObj*)usbBulkHandle)->usbBulkInfo.channelInfo.endPointIn,
                            buf, len, &actualLen, -1);
                        if (ret < 0) {
                            USER_LOG_ERROR("Read usb bulk data failed, errno = %d", ret);
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                        }

                        *realLen = actualLen;
                    } else {
                        *realLen = read(((T_HalUsbBulkObj*)usbBulkHandle)->ep2, buf, len);
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UsbBulkGetDeviceInfo =
                    +[](T_DjiHalUsbBulkDeviceInfo* deviceInfo) -> T_DjiReturnCode {
                    auto& parameter = PARAMETER;
                    auto& usb = parameter.usb;
                    //attention: this interface only be called in usb device mode.
                    deviceInfo->vid = usb.vid;
                    deviceInfo->pid = usb.pid;

                    // This bulk channel is used to obtain DJI camera video stream and push 3rd-party camera video stream.
                    deviceInfo->channelInfo[DJI_HAL_USB_BULK_NUM_0].interfaceNum =
                        usb.bulk1.interface_num;
                    deviceInfo->channelInfo[DJI_HAL_USB_BULK_NUM_0].endPointIn =
                        usb.bulk1.endpoint_in;
                    deviceInfo->channelInfo[DJI_HAL_USB_BULK_NUM_0].endPointOut =
                        usb.bulk1.endpoint_out;

                    // This bulk channel is used to obtain DJI perception image and download camera media file.
                    deviceInfo->channelInfo[DJI_HAL_USB_BULK_NUM_1].interfaceNum =
                        usb.bulk2.interface_num;
                    deviceInfo->channelInfo[DJI_HAL_USB_BULK_NUM_1].endPointIn =
                        usb.bulk2.endpoint_in;
                    deviceInfo->channelInfo[DJI_HAL_USB_BULK_NUM_1].endPointOut =
                        usb.bulk2.endpoint_out;

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
            };

            T_DjiFileSystemHandler fileSystemHandler = {
                .FileOpen = +[](const char* fileName, const char* fileMode,
                                T_DjiFileHandle* fileObj) -> T_DjiReturnCode {
                    if (fileName == NULL || fileMode == NULL || fileObj == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    *fileObj = fopen(fileName, fileMode);
                    if (*fileObj == NULL) {
                        goto out;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

                out:
                    free(*fileObj);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                },
                .FileClose = +[](T_DjiFileHandle fileObj) -> T_DjiReturnCode {
                    int32_t ret;

                    if (fileObj == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = fclose((FILE*)fileObj);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .FileWrite = +[](T_DjiFileHandle fileObj, const uint8_t* buf, uint32_t len,
                                 uint32_t* realLen) -> T_DjiReturnCode {
                    int32_t ret;

                    if (fileObj == NULL || buf == NULL || len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = fwrite(buf, 1, len, (FILE*)fileObj);
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .FileRead = +[](T_DjiFileHandle fileObj, uint8_t* buf, uint32_t len,
                                uint32_t* realLen) -> T_DjiReturnCode {
                    int32_t ret;

                    if (fileObj == NULL || buf == NULL || len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = fread(buf, 1, len, (FILE*)fileObj);
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .FileSeek = +[](T_DjiFileHandle fileObj, uint32_t offset) -> T_DjiReturnCode {
                    int32_t ret;

                    if (fileObj == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = fseek((FILE*)fileObj, offset, SEEK_SET);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .FileSync = +[](T_DjiFileHandle fileObj) -> T_DjiReturnCode {
                    int32_t ret;

                    if (fileObj == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = fflush((FILE*)fileObj);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .DirOpen = +[](const char* filePath, T_DjiDirHandle* dirObj) -> T_DjiReturnCode {
                    if (filePath == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    *dirObj = opendir(filePath);
                    if (*dirObj == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .DirClose = +[](T_DjiDirHandle dirObj) -> T_DjiReturnCode {
                    int32_t ret;

                    if (dirObj == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = closedir((DIR*)dirObj);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .DirRead = +[](T_DjiDirHandle dirObj, T_DjiFileInfo* fileInfo) -> T_DjiReturnCode {
                    struct dirent* dirent;

                    if (dirObj == NULL || fileInfo == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    dirent = readdir((DIR*)dirObj);
                    if (!dirent) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    if (dirent->d_type == DT_DIR) {
                        fileInfo->isDir = true;
                    } else {
                        fileInfo->isDir = false;
                    }
                    strcpy(fileInfo->path, dirent->d_name);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .Mkdir = +[](const char* filePath) -> T_DjiReturnCode {
                    int32_t ret;

                    if (filePath == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = mkdir(filePath, S_IRWXU);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .Unlink = +[](const char* filePath) -> T_DjiReturnCode {
                    int32_t ret;

                    if (filePath == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    if (filePath[strlen(filePath) - 1] == '/') {
                        ret = rmdir(filePath);
                        if (ret < 0) {
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                        }
                    } else {
                        ret = unlink(filePath);
                        if (ret < 0) {
                            return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                        }
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .Rename = +[](const char* oldFilePath, const char* newFilePath) -> T_DjiReturnCode {
                    int32_t ret;

                    if (oldFilePath == NULL || newFilePath == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = rename(oldFilePath, newFilePath);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .Stat = +[](const char* filePath, T_DjiFileInfo* fileInfo) -> T_DjiReturnCode {
                    struct stat st;
                    int32_t ret;
                    struct tm* fileTm;

                    if (filePath == NULL || fileInfo == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = stat(filePath, &st);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    fileTm = localtime((const time_t*)&(st.st_mtim));
                    if (fileTm == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    fileInfo->size = st.st_size;

                    fileInfo->createTime.year = fileTm->tm_year + 1900 - 1980;
                    fileInfo->createTime.month = fileTm->tm_mon + 1;
                    fileInfo->createTime.day = fileTm->tm_mday;
                    fileInfo->createTime.hour = fileTm->tm_hour;
                    fileInfo->createTime.minute = fileTm->tm_min;
                    fileInfo->createTime.second = fileTm->tm_sec;

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
            };

            T_DjiSocketHandler socketHandler = {
                .Socket =
                    +[](E_DjiSocketMode mode, T_DjiSocketHandle* socketHandle) -> T_DjiReturnCode {
                    T_SocketHandleStruct* socketHandleStruct;
                    socklen_t optlen = sizeof(int);
                    int rcvBufSize = SOCKET_RECV_BUF_MAX_SIZE;
                    int opt = 1;
                    int result = 0;

                    /*! set the socket default read buffer to 20MByte */
                    result = system("echo 20000000 > /proc/sys/net/core/rmem_default");
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    /*! set the socket max read buffer to 50MByte */
                    result = system("echo 50000000 > /proc/sys/net/core/rmem_max");
                    if (result != 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    if (socketHandle == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    socketHandleStruct =
                        (T_SocketHandleStruct*)malloc(sizeof(T_SocketHandleStruct));
                    if (socketHandleStruct == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
                    }

                    if (mode == DJI_SOCKET_MODE_UDP) {
                        socketHandleStruct->socketFd = socket(PF_INET, SOCK_DGRAM, 0);

                        if (setsockopt(socketHandleStruct->socketFd, SOL_SOCKET, SO_REUSEADDR, &opt,
                                       optlen) < 0) {
                            goto out;
                        }

                        if (setsockopt(socketHandleStruct->socketFd, SOL_SOCKET, SO_RCVBUF,
                                       &rcvBufSize, optlen) < 0) {
                            goto out;
                        }
                    } else if (mode == DJI_SOCKET_MODE_TCP) {
                        socketHandleStruct->socketFd = socket(PF_INET, SOCK_STREAM, 0);
                    } else {
                        goto out;
                    }

                    *socketHandle = socketHandleStruct;

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

                out:
                    close(socketHandleStruct->socketFd);
                    free(socketHandleStruct);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                },
                .Close = +[](T_DjiSocketHandle socketHandle) -> T_DjiReturnCode {
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    int32_t ret;

                    if (socketHandleStruct->socketFd <= 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = close(socketHandleStruct->socketFd);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    free(socketHandle);

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .Bind = +[](T_DjiSocketHandle socketHandle, const char* ipAddr,
                            uint32_t port) -> T_DjiReturnCode {
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    struct sockaddr_in addr;
                    int32_t ret;

                    if (socketHandle == NULL || ipAddr == NULL || port == 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    bzero(&addr, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(port);
                    addr.sin_addr.s_addr = inet_addr(ipAddr);

                    ret = bind(socketHandleStruct->socketFd, (struct sockaddr*)&addr,
                               sizeof(struct sockaddr_in));
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UdpSendData =
                    +[](T_DjiSocketHandle socketHandle, const char* ipAddr, uint32_t port,
                        const uint8_t* buf, uint32_t len, uint32_t* realLen) -> T_DjiReturnCode {
                    struct sockaddr_in addr;
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    int32_t ret;

                    if (socketHandle == NULL || ipAddr == NULL || port == 0 || buf == NULL ||
                        len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    bzero(&addr, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(port);
                    addr.sin_addr.s_addr = inet_addr(ipAddr);

                    ret = sendto(socketHandleStruct->socketFd, buf, len, 0, (struct sockaddr*)&addr,
                                 sizeof(struct sockaddr_in));
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .UdpRecvData =
                    +[](T_DjiSocketHandle socketHandle, char* ipAddr, uint32_t* port, uint8_t* buf,
                        uint32_t len, uint32_t* realLen) -> T_DjiReturnCode {
                    struct sockaddr_in addr;
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    uint32_t addrLen = 0;
                    int32_t ret;

                    if (socketHandle == NULL || ipAddr == NULL || port == 0 || buf == NULL ||
                        len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = recvfrom(socketHandleStruct->socketFd, buf, len, 0,
                                   (struct sockaddr*)&addr, &addrLen);
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .TcpListen = +[](T_DjiSocketHandle socketHandle) -> T_DjiReturnCode {
                    int32_t ret;
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;

                    if (socketHandle == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = listen(socketHandleStruct->socketFd, 5);
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .TcpAccept = +[](T_DjiSocketHandle socketHandle, char* ipAddr, uint32_t* port,
                                 T_DjiSocketHandle* outSocketHandle) -> T_DjiReturnCode {
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    T_SocketHandleStruct* outSocketHandleStruct;
                    struct sockaddr_in addr;
                    uint32_t addrLen = 0;

                    if (socketHandle == NULL || ipAddr == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    outSocketHandleStruct =
                        (T_SocketHandleStruct*)malloc(sizeof(T_SocketHandleStruct));
                    if (outSocketHandleStruct == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_MEMORY_ALLOC_FAILED;
                    }

                    outSocketHandleStruct->socketFd =
                        accept(socketHandleStruct->socketFd, (struct sockaddr*)&addr, &addrLen);
                    if (outSocketHandleStruct->socketFd < 0) {
                        free(outSocketHandleStruct);
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    *port = ntohs(addr.sin_port);
                    *outSocketHandle = outSocketHandleStruct;

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .TcpConnect = +[](T_DjiSocketHandle socketHandle, const char* ipAddr,
                                  uint32_t port) -> T_DjiReturnCode {
                    struct sockaddr_in addr;
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    int32_t ret;

                    if (socketHandle == NULL || ipAddr == NULL || port == 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    bzero(&addr, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(port);
                    addr.sin_addr.s_addr = inet_addr(ipAddr);

                    ret = connect(socketHandleStruct->socketFd, (struct sockaddr*)&addr,
                                  sizeof(struct sockaddr_in));
                    if (ret < 0) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .TcpSendData = +[](T_DjiSocketHandle socketHandle, const uint8_t* buf, uint32_t len,
                                   uint32_t* realLen) -> T_DjiReturnCode {
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    int32_t ret;

                    if (socketHandle == NULL || buf == NULL || len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = send(socketHandleStruct->socketFd, buf, len, 0);
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
                .TcpRecvData = +[](T_DjiSocketHandle socketHandle, uint8_t* buf, uint32_t len,
                                   uint32_t* realLen) -> T_DjiReturnCode {
                    T_SocketHandleStruct* socketHandleStruct = (T_SocketHandleStruct*)socketHandle;
                    int32_t ret;

                    if (socketHandle == NULL || buf == NULL || len == 0 || realLen == NULL) {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    ret = recv(socketHandleStruct->socketFd, buf, len, 0);
                    if (ret >= 0) {
                        *realLen = ret;
                    } else {
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                },
            };

            result = DjiPlatform_RegOsalHandler(&osalHandler);
            if (result != 0) {
                qError("DjiPlatform_RegOsalHandler return {:#x}!", result);
                break;
            }

            result = DjiLogger_AddConsole(&printConsole);
            if (result != 0) {
                qError("DjiLogger_AddConsole return {:#x}!", result);
                break;
            }

            result = DjiLogger_AddConsole(&printFile);
            if (result != 0) {
                qError("DjiLogger_AddConsole return {:#x}!", result);
                break;
            }

            result = DjiPlatform_RegHalUartHandler(&uartHandler);
            if (result != 0) {
                qError("DjiPlatform_RegHalUartHandler return {:#x}!", result);
                break;
            }

            if (parameter.connect_type == 1) {
                result = DjiPlatform_RegHalUsbBulkHandler(&usbBulkHandler);
                if (result != 0) {
                    qError("DjiPlatform_RegHalUsbBulkHandler return {:#x}!", result);
                    break;
                }
                qTrace("DjiPlatform_RegHalUsbBulkHandler success!");
            } else if (parameter.connect_type == 2) {
                result = DjiPlatform_RegHalNetworkHandler(&networkHandler);
                if (result != 0) {
                    qError("DjiPlatform_RegHalNetworkHandler return {:#x}!", result);
                    break;
                }
                qTrace("Register Network handler success!");
            }

            //Attention: if you want to use camera stream view function, please uncomment it.
            result = DjiPlatform_RegSocketHandler(&socketHandler);
            if (result != 0) {
                qError("DjiPlatform_RegSocketHandler return {:#x}!", result);
                break;
            }

            result = DjiPlatform_RegFileSystemHandler(&fileSystemHandler);
            if (result != 0) {
                qError("DjiPlatform_RegFileSystemHandler return {:#x}!", result);
                break;
            }

            T_DjiUserInfo user_info{};
            std::snprintf(user_info.appName, sizeof(user_info.appName), "%s",
                          parameter.app_name.c_str());
            std::snprintf(user_info.appId, sizeof(user_info.appId), "%s", parameter.app_id.c_str());
            std::snprintf(user_info.appKey, sizeof(user_info.appKey), "%s",
                          parameter.app_key.c_str());
            std::snprintf(user_info.appLicense, sizeof(user_info.appLicense), "%s",
                          parameter.app_license.c_str());
            std::snprintf(user_info.developerAccount, sizeof(user_info.developerAccount), "%s",
                          parameter.developer_account.c_str());
            std::snprintf(user_info.baudRate, sizeof(user_info.baudRate), "%s",
                          parameter.baud_rate.c_str());
            result = DjiCore_Init(&user_info);
            if (result != 0) {
                qError("DjiCore_Init return {:#x}!", result);
                break;
            }

            T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo;
            result = DjiAircraftInfo_GetBaseInfo(&aircraftInfoBaseInfo);
            if (result != 0) {
                qError("DjiAircraftInfo_GetBaseInfo return {:#x}!", result);
                break;
            }

            if (aircraftInfoBaseInfo.mountPositionType != DJI_MOUNT_POSITION_TYPE_PAYLOAD_PORT) {
                T_DjiAircraftVersion aircraftInfoVersion;
                result = DjiAircraftInfo_GetAircraftVersion(&aircraftInfoVersion);
                if (result != 0) {
                    qError("DjiAircraftInfo_GetAircraftVersion return {:#x}!", result);
                } else {
                    qInfo("Aircraft version is V{}.{}.{}.{}", aircraftInfoVersion.majorVersion,
                          aircraftInfoVersion.minorVersion, aircraftInfoVersion.modifyVersion,
                          aircraftInfoVersion.debugVersion);
                }
            }

            result = DjiCore_SetAlias("PSDK_APPALIAS");
            if (result != 0) {
                qError("DjiCore_SetAlias return {:#x}!", result);
                break;
            }

            T_DjiFirmwareVersion firmwareVersion = {
                .majorVersion = 1,
                .minorVersion = 0,
                .modifyVersion = 0,
                .debugVersion = 0,
            };
            result = DjiCore_SetFirmwareVersion(firmwareVersion);
            if (result != 0) {
                qError("DjiCore_SetFirmwareVersion return {:#x}!", result);
                break;
            }

            result = DjiCore_SetSerialNumber("PSDK12345678XX");
            if (result != 0) {
                qError("DjiCore_SetSerialNumber return {:#x}!", result);
                break;
            }

            result = DjiCore_ApplicationStart();
            if (result != 0) {
                qError("DjiCore_ApplicationStart return {:#x}!", result);
                break;
            }
        } while (false);

        return result;
    }

protected:
    friend class ref_singleton<self>;
};

};  // namespace

object::ptr make(init_parameter const& parameter) {
    return ref_singleton<register2>::make(parameter);
}

namespace __flight_control__ {

struct impl : public object {
    using self = flight_control;

    class register2 final : public object {
    public:
        using self = register2;
        using ptr = std::shared_ptr<self>;

        register2() {
            T_DjiReturnCode return_code{0u};

            return_code = DjiFlightController_Init(T_DjiFlightControllerRidInfo{
                .latitude = 22.542812,
                .longitude = 113.958902,
                .altitude = 10,
            });
            THROW_EXCEPTION(return_code == 0, "DjiFlightController_Init return {:#x}", return_code);

            return_code = DjiWaypointV2_Init();
            THROW_EXCEPTION(return_code == 0, "DjiWaypointV2_Init return {:#x}", return_code);

            qTrace("Flight Control Init!");
        }

        ~register2() {
            T_DjiReturnCode return_code{0u};

            return_code = DjiFlightController_DeInit();
            if (0 != return_code) {
                qError("DjiFlightController_DeInit return {:#x}!", return_code);
            }

            return_code = DjiWaypointV2_Deinit();
            if (0 != return_code) {
                qError("DjiWaypointV2_Deinit return {:#x}!", return_code);
            }

            qTrace("Flight Control Deinit!");
        }

    protected:
        friend class ref_singleton<self>;
    };

    psdk::register2::ptr psdk_register_ptr;
    register2::ptr register_ptr;
    self::init_parameter init_parameter;

    std::promise<void> promise;
    std::future<void> future;
    std::atomic_bool exit{false};
    std::mutex mutex;
    std::condition_variable condition;
    self::action action;
    self::parameter::ptr parameter_ptr;
    std::function<void(int32_t)> callback{nullptr};
    uint32_t mission_id{0u};
    T_DjiWayPointV2MissionSettings mission;

    ~impl() {
        exit = true;
        condition.notify_all();
        future.get();
    }

    static E_DJIWaypointV2MissionFinishedAction finish_action(self::action action) {
        static std::map<self::action, E_DJIWaypointV2MissionFinishedAction> map{
            {self::action::no_action, DJI_WAYPOINT_V2_FINISHED_NO_ACTION},
            {self::action::go_home, DJI_WAYPOINT_V2_FINISHED_GO_HOME},
            {self::action::auto_land, DJI_WAYPOINT_V2_FINISHED_AUTO_LANDING},
        };
        E_DJIWaypointV2MissionFinishedAction result{DJI_WAYPOINT_V2_FINISHED_NO_ACTION};
        auto it = map.find(action);
        if (it != map.end()) {
            result = it->second;
        }
        return result;
    }

    auto mission_settings(self::waypoints_parameter::ptr const& parameter_ptr) {
        T_DjiWayPointV2MissionSettings mission_settings{0};

        T_DJIWaypointV2ActionList action_list = {nullptr, 0};
        T_DjiFcSubscriptionPositionFused position{};
        T_DjiDataTimestamp timestamp{};
        DjiFcSubscription_GetLatestValueOfTopic(
            DJI_FC_SUBSCRIPTION_TOPIC_POSITION_FUSED, (uint8_t*)&position,
            sizeof(T_DjiFcSubscriptionPositionFused), &timestamp);

        T_DjiWaypointV2* waypoint_list = static_cast<T_DjiWaypointV2*>(
            malloc((parameter_ptr->points.size() + 1) * sizeof(T_DjiWaypointV2)));
        waypoint_list[0] = T_DjiWaypointV2{
            .longitude = position.longitude,
            .latitude = position.latitude,
            .relativeHeight = init_parameter.global_flight_altitude,
            .waypointType = DJI_WAYPOINT_V2_FLIGHT_PATH_MODE_GO_TO_POINT_IN_STRAIGHT_AND_STOP,
            .headingMode = DJI_WAYPOINT_V2_HEADING_MODE_AUTO,
            .config = {.useLocalCruiseVel = 0, .useLocalMaxVel = 0},
            .dampingDistance = 40,
            .heading = 0,
            .turnMode = DJI_WAYPOINT_V2_TURN_MODE_CLOCK_WISE,
            .pointOfInterest = {.positionX = 0, .positionY = 0, .positionZ = 0},
            .maxFlightSpeed = init_parameter.global_flight_speed_max,
            .autoFlightSpeed = init_parameter.global_flight_speed,
        };
        for (auto i = 0u; i < parameter_ptr->points.size(); ++i) {
            auto& waypoint = parameter_ptr->points[i];
            waypoint_list[i + 1] = T_DjiWaypointV2{
                .longitude = waypoint.longitude * M_PI / 180.0,
                .latitude = waypoint.latitude * M_PI / 180.0,
                .relativeHeight = static_cast<float32_t>(waypoint.relative_height),
                .waypointType = DJI_WAYPOINT_V2_FLIGHT_PATH_MODE_GO_TO_POINT_IN_STRAIGHT_AND_STOP,
                .headingMode = DJI_WAYPOINT_V2_HEADING_MODE_AUTO,
                .config = {.useLocalCruiseVel = 0, .useLocalMaxVel = 0},
                .dampingDistance = 40,
                .heading = 0,
                .turnMode = DJI_WAYPOINT_V2_TURN_MODE_CLOCK_WISE,
                .pointOfInterest = {.positionX = 0, .positionY = 0, .positionZ = 0},
                .maxFlightSpeed = init_parameter.global_flight_speed_max,
                .autoFlightSpeed = init_parameter.global_flight_speed,
            };
        }

        mission_settings.missionID = (++mission_id);
        mission_settings.repeatTimes = parameter_ptr->repeat_times;
        mission_settings.finishedAction = finish_action(parameter_ptr->finish_action);
        mission_settings.maxFlightSpeed =
            static_cast<float>(init_parameter.global_flight_speed_max);
        mission_settings.autoFlightSpeed = static_cast<float>(init_parameter.global_flight_speed);
        mission_settings.actionWhenRcLost = DJI_WAYPOINT_V2_MISSION_KEEP_EXECUTE_WAYPOINT_V2;
        mission_settings.gotoFirstWaypointMode =
            DJI_WAYPOINT_V2_MISSION_GO_TO_FIRST_WAYPOINT_MODE_POINT_TO_POINT;
        mission_settings.mission = waypoint_list;
        mission_settings.missTotalLen = parameter_ptr->points.size() + 1;
        mission_settings.actionList = action_list;

        return mission_settings;
    }

    int32_t handle_takeoff_or_land(self::action action) {
        int32_t result{0};

        do {
            if (action != self::action::take_off || action != self::action::land) {
                result = self::unknown_error;
                break;
            }

            T_DjiReturnCode return_code{0u};
            T_DjiFcSubscriptionFlightStatus status{DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_STOPED};
            T_DjiDataTimestamp timestamp{0};

            // return_code = DjiFlightController_RegJoystickCtrlAuthorityEventCallback(
            //     +[](T_DjiFlightControllerJoystickCtrlAuthorityEventInfo event) -> T_DjiReturnCode {
            //         return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
            //     });
            // if (0 != return_code) {
            //     qError("DjiFlightController_RegJoystickCtrlAuthorityEventCallback return {:#x}",
            //            return_code);
            //     break;
            // }

            return_code = DjiFlightController_ObtainJoystickCtrlAuthority();
            if (0 != return_code) {
                qError("DjiFlightController_ObtainJoystickCtrlAuthority return {:#x}", return_code);
            }

            DjiFcSubscription_GetLatestValueOfTopic(
                DJI_FC_SUBSCRIPTION_TOPIC_STATUS_FLIGHT, (uint8_t*)&status,
                sizeof(T_DjiFcSubscriptionFlightStatus), &timestamp);
            if (action == self::action::take_off) {
                if (status != DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR) {
                    return_code = DjiFlightController_StartTakeoff();
                    if (0 == return_code) {
                        for (auto retry = 0u; status != DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR &&
                             retry < init_parameter.retry_max;
                             ++retry) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            DjiFcSubscription_GetLatestValueOfTopic(
                                DJI_FC_SUBSCRIPTION_TOPIC_STATUS_FLIGHT, (uint8_t*)&status,
                                sizeof(T_DjiFcSubscriptionFlightStatus), &timestamp);
                        }
                        if (status != DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR) {
                            qError("DjiFlightController_StartTakeoff Timeout!");
                            result = self::result::timeout;
                        } else {
                            qInfo("Action(Take Off) Success!");
                            result = self::result::ok;
                        }
                    } else {
                        qError("DjiFlightController_StartTakeoff return {:#x}", return_code);
                        result = self::result::unknown_error;
                    }
                } else {
                    qError("Current status is DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR!");
                    result = self::result::already_in_air;
                }
            } else if (action == self::action::land) {
                if (status == DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR) {
                    return_code = DjiFlightController_StartLanding();
                    if (0 != return_code) {
                        qError("DjiFlightController_StartLanding return {:#x}", return_code);
                        result = self::result::unknown_error;
                    } else {
                        for (auto retry = 0u;
                             (status == DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR) &&
                             (retry < init_parameter.retry_max);
                             ++retry) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            DjiFcSubscription_GetLatestValueOfTopic(
                                DJI_FC_SUBSCRIPTION_TOPIC_STATUS_FLIGHT, (uint8_t*)&status,
                                sizeof(T_DjiFcSubscriptionFlightStatus), &timestamp);
                        }
                        if (status != DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR) {
                            qInfo("Action(Land) Success!");
                            result = self::result::ok;
                        } else {
                            qError("DjiFlightController_StartLanding Timeout!");
                            result = self::result::timeout;
                        }
                    }
                } else {
                    qError("Current status is not DJI_FC_SUBSCRIPTION_FLIGHT_STATUS_IN_AIR!");
                    result = self::result::already_landed;
                }
            }

            return_code = DjiFlightController_ReleaseJoystickCtrlAuthority();
            if (0 != return_code) {
                qError("DjiFlightController_ObtainJoystickCtrlAuthority return {:#x}", return_code);
            }
        } while (false);

        return result;
    }

    int32_t handle_waypoints(self::waypoints_parameter::ptr const& parameter_ptr) {
        int32_t result{0};

        do {
            if (parameter_ptr == nullptr) {
                qError("parameter_ptr is nullptr!");
                result = self::result::unknown_error;
                break;
            }

            qTrace("Parameter: {}", *parameter_ptr);
            if (parameter_ptr->points.empty()) {
                qError("waypoints is empty!");
                result = self::result::unknown_error;
                break;
            }

            T_DjiFcSubscriptionPositionFused position;
            T_DjiDataTimestamp timestamp;
            DjiFcSubscription_GetLatestValueOfTopic(
                DJI_FC_SUBSCRIPTION_TOPIC_POSITION_FUSED, (uint8_t*)&position,
                sizeof(T_DjiFcSubscriptionPositionFused), &timestamp);
            if (position.visibleSatelliteNumber < init_parameter.number_of_valid_satellites) {
                qError("number_of_satellites={}, number_of_valid_satellites={}",
                       position.visibleSatelliteNumber, init_parameter.number_of_valid_satellites);
                result = self::result::no_satellite;
                break;
            }

            T_DjiReturnCode return_code{0u};
            auto mission = mission_settings(parameter_ptr);
            return_code = DjiWaypointV2_UploadMission(&mission);
            if (return_code != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                qError("DjiWaypointV2_UploadMission return {:#x}!", return_code);
                result = self::unknown_error;
                break;
            }

            promise = std::promise<void>();
            auto f = promise.get_future();

            return_code = DjiWaypointV2_Start();
            if (return_code != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                qError("DjiWaypointV2_Start return {:#x}!", return_code);
                result = self::unknown_error;
                break;
            }

            f.get();

            return_code = DjiWaypointV2_Stop();
            if (return_code != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                qError("DjiWaypointV2_Stop return {:#x}!", return_code);
            }
        } while (false);

        return result;
    }
};

};  // namespace __flight_control__

int32_t flight_control::init(init_parameter const& parameter) {
    int32_t result{0};

    do {
        using namespace __flight_control__;
        auto impl_ptr = std::make_shared<impl>();

        impl_ptr->psdk_register_ptr = ref_singleton<psdk::register2>::make();
        impl_ptr->register_ptr = ref_singleton<impl::register2>::make();

        result = DjiFlightController_SetRCLostAction(DJI_FLIGHT_CONTROLLER_RC_LOST_ACTION_GOHOME);
        if (0 != result) {
            qError("DjiFlightController_SetRCLostAction return {:#x}", result);
        }

        result = DjiWaypointV2_RegisterMissionEventCallback(
            +[](T_DjiWaypointV2MissionEventPush event) -> T_DjiReturnCode {
                qTrace("DjiWaypointV2_RegisterMissionEventCallback: event {:#x}, "
                       "timestamp: {}",
                       event.event, event.FCTimestamp);
                return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
            });
        if (0 != result) {
            qError("DjiWaypointV2_RegisterMissionEventCallback return {:#x}", result);
        }

        result = DjiWaypointV2_RegisterMissionStateCallback(
            +[](T_DjiWaypointV2MissionStatePush state) -> T_DjiReturnCode {
                if (state.state == DJI_WAYPOINT_V2_MISSION_STATE_EXIT_MISSION) {
                    auto ref = ref_singleton<self>::make();
                    auto impl_ptr = std::static_pointer_cast<impl>(ref->impl_ptr);
                    if (impl_ptr != nullptr) {
                        impl_ptr->promise.set_value();
                    }
                }
                return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
            });
        if (0 != result) {
            qError("DjiWaypointV2_RegisterMissionStateCallback return {:#x}", result);
            break;
        }

        impl_ptr->init_parameter = parameter;
        impl_ptr->future = std::async(
            std::launch::async,
            [](impl* impl_ptr) -> void {
                self::action action{self::action::no_action};
                self::parameter::ptr parameter_ptr{nullptr};

                while (!impl_ptr->exit) {
                    {
                        std::unique_lock<std::mutex> lock(impl_ptr->mutex);
                        impl_ptr->condition.wait(lock, [impl_ptr] {
                            return impl_ptr->exit || impl_ptr->action != self::action::no_action;
                        });
                        if (impl_ptr->exit) {
                            break;
                        }
                        action = impl_ptr->action;
                        impl_ptr->action = self::action::no_action;
                        parameter_ptr = std::move(impl_ptr->parameter_ptr);
                    }

                    int32_t result{0u};
                    qInfo("Receive Action({})!", static_cast<uint32_t>(action));
                    if (!impl_ptr->init_parameter.simulator) {
                        if (action == self::action::take_off || action == self::action::land) {
                            result = impl_ptr->handle_takeoff_or_land(action);
                        } else if (action == self::action::waypoints) {
                            result = impl_ptr->handle_waypoints(
                                std::static_pointer_cast<self::waypoints_parameter>(parameter_ptr));
                        } else {
                            result = self::result::unknown_error;
                        }
                    }
                    qInfo("Action({}) Finished!", static_cast<uint32_t>(action));

                    if (impl_ptr->callback) {
                        impl_ptr->callback(result);
                    }
                }
            },
            impl_ptr.get());

        self::impl_ptr = impl_ptr;
    } while (false);

    return result;
}

int32_t flight_control::set_action(action action,
                                   parameter::ptr const& parameter_ptr,
                                   std::function<void(int32_t)> const& callback) {
    int32_t result{self::result::ok};

    do {
        using namespace __flight_control__;
        auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
        if (impl_ptr == nullptr) {
            result = self::result::unknown_error;
            break;
        }

        if (callback) {
            std::lock_guard<std::mutex> lock(impl_ptr->mutex);
            impl_ptr->action = action;
            impl_ptr->callback = callback;
            impl_ptr->parameter_ptr = parameter_ptr;
            impl_ptr->condition.notify_one();
        } else {
            auto p = std::make_shared<std::promise<int32_t>>();
            auto f = p->get_future();

            {
                std::lock_guard<std::mutex> lock(impl_ptr->mutex);
                impl_ptr->action = action;
                impl_ptr->callback = [p](int32_t result) { p->set_value(result); };
                impl_ptr->parameter_ptr = parameter_ptr;
                impl_ptr->condition.notify_one();
            }

            result = f.get();
        }

    } while (false);

    return result;
}

namespace __camera__ {
struct impl final : public object {
    using self = camera;

    static inline std::map<self::index, uint32_t> map{
        {self::index::fpv, DJI_LIVEVIEW_CAMERA_POSITION_FPV},
        {self::index::h30t, DJI_LIVEVIEW_CAMERA_POSITION_NO_1}};

    class register2 : public object {
    protected:
        friend class ref_singleton<register2>;

    public:
        using self = register2;
        using ptr = std::shared_ptr<self>;

        register2() {
            T_DjiReturnCode result{DjiLiveview_Init()};
            THROW_EXCEPTION(result == DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS,
                            "DjiLiveview_Init return {:#x}!", result);
            qTrace("Camera Init!");
        }

        ~register2() {
            T_DjiReturnCode result{DjiLiveview_Deinit()};
            if (0 != result) {
                qWarn("DjiLiveview_Deinit return {:#x}!", result);
            } else {
                qTrace("Camera Deinit!");
            }
        }
    };

    register2::ptr register_ptr{nullptr};
    self::init_parameter init_parameter;
    std::map<
        uint32_t,
        std::tuple<std::function<void(self::frame&&)>, std::queue<std::shared_ptr<self::frame>>>>
        callbacks;

    std::mutex mutex;
    std::condition_variable cond_var;
    std::future<void> future;
    std::atomic_bool exit{false};

    ~impl() {
        exit = true;
        cond_var.notify_all();
        future.wait();
    }
};
};  // namespace __camera__

int32_t camera::init(init_parameter const& parameter) {
    int32_t result{0};

    do {
        using namespace __camera__;
        auto impl_ptr = std::make_shared<impl>();

        impl_ptr->register_ptr = ref_singleton<impl::register2>::make();
        impl_ptr->init_parameter = parameter;
        impl_ptr->future = std::async(
            std::launch::async,
            [](impl* impl_ptr) {
                qTrace("Camera Thread Enter:");

                while (!impl_ptr->exit) {
                    {
                        std::unique_lock<std::mutex> lock(impl_ptr->mutex);
                        impl_ptr->cond_var.wait(lock);
                    }

                    if (impl_ptr->exit) {
                        break;
                    }

                    for (auto& it : impl_ptr->callbacks) {
                        std::function<void(self::frame&&)> callback;
                        std::queue<std::shared_ptr<self::frame>> frames;

                        {
                            std::lock_guard<std::mutex> lock(impl_ptr->mutex);
                            auto& [_callback, _frames] = it.second;
                            callback = _callback;
                            frames = std::move(_frames);
                        }

                        if (callback != nullptr) {
                            while (!frames.empty()) {
                                auto frame = std::move(frames.front());
                                frames.pop();
                                callback(std::move(*frame));
                            }
                        }
                    }
                }

                for (auto& it : impl_ptr->callbacks) {
                    DjiLiveview_StopH264Stream(static_cast<E_DjiLiveViewCameraPosition>(it.first),
                                               DJI_LIVEVIEW_CAMERA_SOURCE_DEFAULT);
                }

                qTrace("Camera Thread Exit!");
            },
            impl_ptr.get());

        self::impl_ptr = impl_ptr;
    } while (false);

    return result;
}

int32_t camera::subscribe(index index, std::function<void(frame&&)> const& callback) {
    int32_t result{0};

    do {
        using namespace __camera__;
        auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
        if (nullptr == impl_ptr) {
            qError("impl_ptr is nullptr!");
            result = IMPL_NULLPTR;
            break;
        }

        auto it = impl::map.find(index);
        if (it == impl::map.end()) {
            qError("index is invalid!");
            result = PARAM_INVALID;
            break;
        }

        std::lock_guard<std::mutex> lock(impl_ptr->mutex);
        impl_ptr->callbacks[it->second] =
            std::make_tuple(callback, std::queue<std::shared_ptr<self::frame>>{});
        result = DjiLiveview_StartH264Stream(
            static_cast<E_DjiLiveViewCameraPosition>(it->second),
            DJI_LIVEVIEW_CAMERA_SOURCE_DEFAULT,
            +[](E_DjiLiveViewCameraPosition position, uint8_t const* buf, uint32_t len) {
#ifdef DEBUG
                static std::ofstream ofs{[]() -> string {
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    auto file = fmt::format("{:%Y-%m-%d_%H-%M-%S}.h264", fmt::localtime(time));
                    return file;
                }()};

                ofs.write(reinterpret_cast<char const*>(buf), len);
#endif
                auto camera_ptr = ref_singleton<self>::make();
                auto impl_ptr = std::static_pointer_cast<impl>(camera_ptr->impl_ptr);
                if (impl_ptr != nullptr && buf != nullptr) {
                    auto frame_ptr = std::make_shared<std::vector<uint8_t>>(buf, buf + len);
                    std::lock_guard<std::mutex> lock(impl_ptr->mutex);
                    std::get<1>(impl_ptr->callbacks[position]).emplace(frame_ptr);
                    impl_ptr->cond_var.notify_one();
                }
            });
        if (0 != result) {
            qError("DjiLiveview_StartH264Stream return {:#x}!", result);
            result = UNKNOWN_ERROR;
            break;
        }
    } while (false);

    return result;
}

int32_t camera::unsubscribe(index index) {
    int32_t result{0};

    do {
        using namespace __camera__;
        auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
        if (nullptr == impl_ptr) {
            qError("impl_ptr is nullptr!");
            result = IMPL_NULLPTR;
            break;
        }

        auto it = impl::map.find(index);
        if (it == impl::map.end()) {
            qError("index is invalid!");
            result = PARAM_INVALID;
            break;
        }

        std::lock_guard<std::mutex> lock(impl_ptr->mutex);
        impl_ptr->callbacks[it->second] =
            std::make_tuple<std::function<void(frame&&)>, std::queue<std::shared_ptr<self::frame>>>(
                nullptr, {});
        result = DjiLiveview_StopH264Stream(static_cast<E_DjiLiveViewCameraPosition>(it->second),
                                            DJI_LIVEVIEW_CAMERA_SOURCE_DEFAULT);
        if (0 != result) {
            qWarn("DjiLiveview_StopH264Stream return {:#x}!", result);
            result = 0;
            break;
        }
    } while (false);

    return result;
}

camera::parameter camera::get(index index) const {
    static std::map<self::index, self::parameter> parameters{
        {self::index::fpv,
         self::parameter{
             .stream_type = self::parameter::stream::h264,
             .width = 1920u,
             .height = 1080u,
             .fps = 30u,
             .bitrate = 16 * 1000 * 1000u,
             .colorspace = self::parameter::colorspace::yuv420,
         }},
        {self::index::h30t,
         self::parameter{
             .stream_type = self::parameter::stream::h264,
             .width = 1440u,
             .height = 1080u,
             .fps = 30u,
             .bitrate = 16 * 1000 * 1000u,
             .colorspace = self::parameter::colorspace::yuv420,
         }}};

    camera::parameter parameter;

    auto it = parameters.find(index);
    if (it != parameters.end()) {
        parameter = it->second;
    }

    return parameter;
}

namespace __ir_camera__ {
struct impl final : public object {
    using base = object;
    using self = ir_camera;
    using ptr = std::shared_ptr<self>;

    static inline std::map<self::direction, E_DjiPerceptionDirection> map{
        {self::direction::bottom, DJI_PERCEPTION_RECTIFY_DOWN},
        {self::direction::top, DJI_PERCEPTION_RECTIFY_UP},
        {self::direction::left, DJI_PERCEPTION_RECTIFY_LEFT},
        {self::direction::right, DJI_PERCEPTION_RECTIFY_RIGHT},
        {self::direction::front, DJI_PERCEPTION_RECTIFY_FRONT},
        {self::direction::back, DJI_PERCEPTION_RECTIFY_REAR},
    };

    class register2 : public object {
    public:
        using base = object;
        using self = register2;
        using ptr = std::shared_ptr<self>;

        register2() {
            T_DjiReturnCode result{DjiPerception_Init()};
            THROW_EXCEPTION(0 == result, "init return {:#x}... ", result);
            qTrace("IR Camera Init!");
        }

        ~register2() {
            T_DjiReturnCode result{DjiPerception_Deinit()};
            if (0 != result) {
                qWarn("DjiPerception_Deinit return {:#x}", result);
            } else {
                qTrace("IR Camera Deinit!");
            }
        }

    protected:
        friend class ref_singleton<self>;
    };

    self::init_parameter init_parameter;
    std::map<uint32_t, std::tuple<std::function<void(self::frame&&)>, std::queue<self::frame>>>
        callbacks;

    std::atomic_bool exit{false};
    register2::ptr register_ptr;
    std::future<void> future;
    std::mutex mutex;
    std::condition_variable cond_var;

    ~impl() {
        exit = true;
        cond_var.notify_all();
        future.get();
    }
};
};  // namespace __ir_camera__

int32_t ir_camera::init(init_parameter const& parameter) {
    int32_t result{0};

    do {
        using namespace __ir_camera__;
        auto impl_ptr = std::make_shared<impl>();

        impl_ptr->register_ptr = ref_singleton<impl::register2>::make();
        impl_ptr->init_parameter = parameter;
        impl_ptr->future = std::async(
            std::launch::async,
            [](impl* impl_ptr) {
                qTrace("IR Camera Thread Enter:");
                while (!impl_ptr->exit) {
                    {
                        std::unique_lock<std::mutex> lock(impl_ptr->mutex);
                        impl_ptr->cond_var.wait(lock);
                    }

                    if (impl_ptr->exit) {
                        break;
                    }

                    for (auto& it : impl_ptr->callbacks) {
                        std::function<void(self::frame&&)> callback;
                        std::queue<self::frame> frames;

                        {
                            std::lock_guard<std::mutex> lock(impl_ptr->mutex);
                            auto& [_callback, _frames] = it.second;
                            callback = _callback;
                            frames = std::move(_frames);
                        }

                        if (callback != nullptr) {
                            while (!frames.empty()) {
                                auto frame = std::move(frames.front());
                                frames.pop();
                                callback(std::move(frame));
                            }
                        }
                    }
                }

                for (auto& it : impl_ptr->callbacks) {
                    DjiPerception_UnsubscribePerceptionImage(
                        static_cast<E_DjiPerceptionDirection>(it.first));
                }

                qTrace("IR Camera Thread Exit!");
            },
            impl_ptr.get());

        self::impl_ptr = impl_ptr;
    } while (false);

    return result;
}

int32_t ir_camera::subscribe(self::direction direction,
                             std::function<void(frame&&)> const& callback) {
    int32_t result{0};

    do {
        using namespace __ir_camera__;
        auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
        if (nullptr == impl_ptr) {
            qTrace("impl_ptr is null!");
            result = IMPL_NULLPTR;
            break;
        }

        auto it = impl::map.find(direction);
        if (it == impl::map.end()) {
            qError("direction is invalid!");
            result = PARAM_INVALID;
            break;
        }

        std::lock_guard<std::mutex> lock(impl_ptr->mutex);
        impl_ptr->callbacks[it->second] = std::make_tuple(callback, std::queue<self::frame>{});
        result = DjiPerception_SubscribePerceptionImage(
            it->second, +[](T_DjiPerceptionImageInfo info, uint8_t* buf, uint32_t len) {
#ifdef DEBUG
                static std::ofstream ofs{[]() -> string {
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    auto file = fmt::format("{:%Y-%m-%d_%H-%M-%S}.gray", fmt::localtime(time));
                    return file;
                }()};

                ofs.write(reinterpret_cast<char*>(buf), len);
#endif
                auto camera_ptr = ref_singleton<self>::make();
                auto impl_ptr = std::static_pointer_cast<impl>(camera_ptr->impl_ptr);
                if (impl_ptr != nullptr && buf != nullptr) {
                    std::vector<uint8_t> data(len);
                    std::memcpy(data.data(), buf, len);
                    std::lock_guard<std::mutex> lock(impl_ptr->mutex);
                    std::get<1>(impl_ptr->callbacks[info.rawInfo.direction])
                        .emplace(self::frame{.data = std::move(data),
                                             .width = info.rawInfo.width,
                                             .height = info.rawInfo.height});
                    impl_ptr->cond_var.notify_one();
                }
            });
        if (0 != result) {
            qError("DjiPerception_SubscribePerceptionImage return {:#x}!", result);
            result = UNKNOWN_ERROR;
            break;
        }
    } while (false);

    return result;
}

int32_t ir_camera::unsubscribe(self::direction direction) {
    int32_t result{0};

    do {
        using namespace __ir_camera__;
        auto impl_ptr = std::static_pointer_cast<impl>(self::impl_ptr);
        if (nullptr == impl_ptr) {
            qTrace("impl_ptr is null!");
            result = IMPL_NULLPTR;
            break;
        }

        auto it = impl::map.find(direction);
        if (it == impl::map.end()) {
            qError("direction is invalid!");
            result = PARAM_INVALID;
            break;
        }

        std::lock_guard<std::mutex> lock(impl_ptr->mutex);
        impl_ptr->callbacks[it->second] =
            std::make_tuple<std::function<void(self::frame&&)>, std::queue<self::frame>>(nullptr,
                                                                                         {});
        result = DjiPerception_UnsubscribePerceptionImage(it->second);
        if (0 != result) {
            qWarn("DjiPerception_UnsubscribePerceptionImage return {:#x}!", result);
            result = 0;
            break;
        }
    } while (false);

    return result;
}

};  // namespace psdk
};  // namespace qlib
