import logging
import sys
from pathlib import Path
from typing import Optional
from datetime import datetime

logger = logging.Logger

trace = logging.DEBUG
debug = logging.DEBUG
info = logging.INFO
warning = logging.WARNING
error = logging.ERROR


class factory:
    def __init__(self):
        self._name = "default"
        self._console = True
        self._console_level = logging.INFO
        self._prefix: Optional[Path] = None
        self._file_level = logging.DEBUG

    def name(self, name: str) -> "factory":
        self._name = name
        return self

    def console(self, enabled: bool = True, level: int = logging.INFO) -> "factory":
        self._console = enabled
        self._console_level = level
        return self

    def file(
        self, prefix: Optional[str | Path] = "logs", level: int = logging.DEBUG
    ) -> "factory":
        if isinstance(prefix, str):
            prefix = Path(prefix)
        self._prefix = prefix
        self._file_level = level
        return self

    def build(self) -> logging.Logger:
        logger = logging.getLogger(self._name)
        logger.setLevel(min(self._console_level, self._file_level))
        logger.handlers.clear()  # 避免重复添加 handler

        formater = logging.Formatter(
            "[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s"
        )

        # 控制台日志处理器
        if self._console:
            console_handler = logging.StreamHandler(sys.stdout)
            console_handler.setLevel(self._console_level)
            console_handler.setFormatter(formater)
            logger.addHandler(console_handler)

        # 文件日志处理器
        if self._prefix is not None:
            self._prefix.mkdir(parents=True, exist_ok=True)
            filename = (
                self._prefix / f"{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.log"
            )

            file_handler = logging.FileHandler(filename)
            file_handler.setLevel(self._file_level)
            file_handler.setFormatter(formater)
            logger.addHandler(file_handler)

        return logger

