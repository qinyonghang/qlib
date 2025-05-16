from qlib import dds
import time


def callback(message: dds.string):
    print("123", message.get())


if __name__ == "__main__":
    publisher = dds.publisher(dds.string(), "topic")
    subscriber = dds.subscriber(dds.string(), "topic", callback)

    while True:
        message = dds.string()
        message.set("Hello World!")
        publisher.publish(message)
        time.sleep(1)
