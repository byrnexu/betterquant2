import logging
import colorlog

# 创建默认的 logger 对象
default_logger = logging.getLogger(__name__)
default_logger.setLevel(logging.DEBUG)

# 创建控制台处理器
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)

# 创建彩色日志格式
log_colors = {
    "DEBUG": "cyan",
    "INFO": "blue",
    "WARNING": "yellow",
    "ERROR": "red",
    "CRITICAL": "bold_red",
}
formatter = colorlog.ColoredFormatter(
    "%(log_color)s%(asctime)s %(levelname)s: %(lineno)s %(message)s",
    datefmt="[%Y%m%d %H:%M:%S.000000]",
    log_colors=log_colors,
)

# 设置彩色日志格式
console_handler.setFormatter(formatter)

# 添加处理器到默认 logger 对象
default_logger.addHandler(console_handler)


def debug(msg, *args, **kwargs):
    default_logger.debug(msg, *args, **kwargs)


def info(msg, *args, **kwargs):
    default_logger.info(msg, *args, **kwargs)


def warning(msg, *args, **kwargs):
    default_logger.warning(msg, *args, **kwargs)


def error(msg, *args, **kwargs):
    default_logger.error(msg, *args, **kwargs)


def critical(msg, *args, **kwargs):
    default_logger.critical(msg, *args, **kwargs)
