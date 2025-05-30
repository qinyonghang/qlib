from qlib import dds
import logging
import time

logging.basicConfig(level=logging.DEBUG)

logger = logging.getLogger("network")
logger.setLevel(logging.DEBUG)

stream = logging.StreamHandler()
stream.setLevel(logging.DEBUG)
stream.setFormatter(
    logging.Formatter("[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s")
)
logger.addHandler(stream)


def string_callback(message: dds.string):
    logger.debug(message.get())


if __name__ == "__main__":
    subscriber_str = dds.subscriber(dds.string(), "aircraft", string_callback)

    while True:
        logger.debug("Waiting for messages...")
        time.sleep(1)
