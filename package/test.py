from qlib import dds
import time


def string_callback(message: dds.string):
    print("Type of message:", type(message))
    print(message.get())


if __name__ == "__main__":
    subscriber_str = dds.subscriber(dds.string(), "aircraft", string_callback)

    while True:
        print("Waiting for messages...")
        time.sleep(1)
