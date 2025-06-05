from qlib import dds
import time


def string_callback(message: dds.string):
    print("string:", message.get())


def int8_callback(message: dds.int8):
    print("int8:", message.get())


def int16_callback(message: dds.int16):
    print("int16:", message.get())


def uint8_callback(message: dds.uint8):
    print("uint8:", message.get())


def uint16_callback(message: dds.uint16):
    print("uint16:", message.get())


def float32_callback(message: dds.float32):
    print("float32:", message.get())


def float64_callback(message: dds.float64):
    print("float64:", message.get())


if __name__ == "__main__":
    publisher_str = dds.publisher(dds.string(), "topic_str")
    subscriber_str = dds.subscriber(dds.string(), "topic_str", string_callback)

    publisher_i8 = dds.publisher(dds.int8(), "topic_i8")
    subscriber_i8 = dds.subscriber(dds.int8(), "topic_i8", int8_callback)

    publisher_i16 = dds.publisher(dds.int16(), "topic_i16")
    subscriber_i16 = dds.subscriber(dds.int16(), "topic_i16", int16_callback)

    publisher_u8 = dds.publisher(dds.uint8(), "topic_u8")
    subscriber_u8 = dds.subscriber(dds.uint8(), "topic_u8", uint8_callback)

    publisher_u16 = dds.publisher(dds.uint16(), "topic_u16")
    subscriber_u16 = dds.subscriber(dds.uint16(), "topic_u16", uint16_callback)

    publisher_f32 = dds.publisher(dds.float32(), "topic_f32")
    subscriber_f32 = dds.subscriber(dds.float32(), "topic_f32", float32_callback)

    publisher_f64 = dds.publisher(dds.float64(), "topic_f64")
    subscriber_f64 = dds.subscriber(dds.float64(), "topic_f64", float64_callback)

    while True:
        msg_str = dds.string()
        msg_str.set("Hello World!")
        publisher_str.publish(msg_str)

        msg_i8 = dds.int8()
        msg_i8.set(-1)
        publisher_i8.publish(msg_i8)

        msg_i16 = dds.int16()
        msg_i16.set(-2)
        publisher_i16.publish(msg_i16)

        msg_u8 = dds.uint8()
        msg_u8.set(255)
        publisher_u8.publish(msg_u8)

        msg_u16 = dds.uint16()
        msg_u16.set(65534)
        publisher_u16.publish(msg_u16)

        msg_f32 = dds.float32()
        msg_f32.set(3.1415926)
        publisher_f32.publish(msg_f32)

        msg_f64 = dds.float64()
        msg_f64.set(2.718281828459045)
        publisher_f64.publish(msg_f64)

        # # 越界测试
        # msg_u8 = dds.uint8()
        # msg_u8.set(300)  # uint8 最大为 255
        # publisher_u8.publish(msg_u8)

        # msg_i8 = dds.int8()
        # msg_i8.set(-129)  # int8 最小为 -128
        # publisher_i8.publish(msg_i8)

        time.sleep(1)
