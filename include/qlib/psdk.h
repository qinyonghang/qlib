#pragma once

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

#include <fstream>
#include <iostream>
#include <map>

#include "dji_aircraft_info.h"
#include "dji_core.h"
#include "dji_logger.h"
#include "dji_platform.h"
#include "logger.h"
#include "object.h"
#include "singleton.h"

namespace qlib {
namespace psdk {

class register2 final : public qlib::object<register2> {
public:
    using self = register2;
    using ptr = std::shared_ptr<self>;

    struct parameter {
        uint32_t connect_type;
        string app_name;
        string app_id;
        string app_key;
        string app_license;
        string developer_account;
        string baud_rate;

        // uart
        std::string uart1;
        std::string uart2;

        // network
        struct {
            string name;
            uint32_t vid;
            uint32_t pid;

            auto to_string() const { return fmt::format("[{},{},{}]", name, vid, pid); }
        } network;

        // usb
        struct {
            uint32_t vid;
            uint32_t pid;
            struct {
                string ep_in;
                string ep_out;
                uint16_t interface_num;
                uint16_t endpoint_in;
                uint16_t endpoint_out;
                auto to_string() const {
                    return fmt::format("[{},{},{},{},{}]", ep_in, ep_out, interface_num,
                                       endpoint_in, endpoint_out);
                }
            } bulk1, bulk2;

            auto to_string() const { return fmt::format("[{},{},{},{}]", vid, pid, bulk1, bulk2); }
        } usb;

        auto to_string() const {
            return fmt::format("[{},{},{},{},{},{},{},{},{},{},{}]", connect_type, app_name, app_id,
                               app_key, app_license, developer_account, baud_rate, uart1, uart2,
                               network, usb);
        }
    };

protected:
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

    static inline parameter PARAMETER;
    static inline constexpr auto SOCKET_RECV_BUF_MAX_SIZE = (1000 * 1000 * 10);

    int32_t init(parameter const& parameter) {
        int32_t result{0};

        do {
            PARAMETER = parameter;

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
                        auto now = std::chrono::system_clock::now();
                        auto time = std::chrono::system_clock::to_time_t(now);
                        auto filename = fmt::format("logs/DJ/DJI_{:%Y-%m-%d_%H-%M-%S}.log",
                                                    fmt::localtime(time));
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
                    T_UartHandleStruct* uartHandleStruct = NULL;
                    struct termios options;
                    struct flock lock;
                    T_DjiReturnCode returnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
                    char* ret = NULL;
                    FILE* fp;

                    std::string uartName;
                    if (uartNum == DJI_HAL_UART_NUM_0) {
                        uartName = register2::PARAMETER.uart1;
                    } else if (uartNum == DJI_HAL_UART_NUM_1) {
                        uartName = register2::PARAMETER.uart2;
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
                    int32_t ret;

                    if (ipAddr == NULL || netMask == NULL) {
                        USER_LOG_ERROR("hal network config param error");
                        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
                    }

                    //Attention: need root permission to config ip addr and netmask.
                    auto& LINUX_NETWORK_DEV = register2::PARAMETER.network.name;
                    auto cmdStr = fmt::format("ifconfig {} up", LINUX_NETWORK_DEV);
                    ret = system(cmdStr.c_str());
                    if (ret != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                        USER_LOG_ERROR("Can't open the network."
                                       "Probably the program not execute with root permission."
                                       "Please use the root permission to execute the program.");
                        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
                    }

                    cmdStr = fmt::format("ifconfig {} {} netmask {}", LINUX_NETWORK_DEV, ipAddr,
                                         netMask);
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
                    auto& network = register2::PARAMETER.network;
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

                        auto& bulk1 = register2::PARAMETER.usb.bulk1;
                        auto& bulk2 = register2::PARAMETER.usb.bulk2;
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
                    auto& usb = register2::PARAMETER.usb;
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

            qTrace("DjiCore_Init success!");
        } while (false);

        return result;
    };

    template <class... Args>
    register2(Args&&... args) {
        int32_t result{init(std::forward<Args>(args)...)};
        THROW_EXCEPTION(0 == result, "init return {}... ", result);
    }

    ~register2() {
        int32_t result{0};
        result = DjiCore_DeInit();
        if (result != 0) {
            qError("DjiCore_DeInit return {:#x}!", result);
        }
    }

    friend class qlib::ref_singleton<self>;
};

};  // namespace psdk
};  // namespace qlib
