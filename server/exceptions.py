from logger import logging

class ExceptionWithLog(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)
        logging.exception(f'Exception has occur {args}')

class GeneralException(ExceptionWithLog):
    pass


class UnkownCode(ExceptionWithLog):
    pass


class ProtocolError(ExceptionWithLog):
    pass


class UnkownClient(ExceptionWithLog):
    pass


class FailedPullMessages(ExceptionWithLog):
    pass


class BrokenConnectionException(ExceptionWithLog):
    pass


class RegistrationException(ExceptionWithLog):
    pass


class VersionError(ExceptionWithLog):
    pass
