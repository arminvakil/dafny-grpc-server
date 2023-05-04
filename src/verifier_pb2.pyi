from google.protobuf.internal import containers as _containers
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Iterable as _Iterable, Mapping as _Mapping, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class CloneAndVerifyRequest(_message.Message):
    __slots__ = ["code", "directoryPath", "modifyingFile", "requestsList"]
    CODE_FIELD_NUMBER: _ClassVar[int]
    DIRECTORYPATH_FIELD_NUMBER: _ClassVar[int]
    MODIFYINGFILE_FIELD_NUMBER: _ClassVar[int]
    REQUESTSLIST_FIELD_NUMBER: _ClassVar[int]
    code: str
    directoryPath: str
    modifyingFile: str
    requestsList: _containers.RepeatedCompositeFieldContainer[VerificationRequest]
    def __init__(self, directoryPath: _Optional[str] = ..., code: _Optional[str] = ..., modifyingFile: _Optional[str] = ..., requestsList: _Optional[_Iterable[_Union[VerificationRequest, _Mapping]]] = ...) -> None: ...

class CreateDir(_message.Message):
    __slots__ = ["owner"]
    OWNER_FIELD_NUMBER: _ClassVar[int]
    owner: str
    def __init__(self, owner: _Optional[str] = ...) -> None: ...

class Empty(_message.Message):
    __slots__ = []
    def __init__(self) -> None: ...

class TmpFolder(_message.Message):
    __slots__ = ["modifyingFile", "owner", "path"]
    MODIFYINGFILE_FIELD_NUMBER: _ClassVar[int]
    OWNER_FIELD_NUMBER: _ClassVar[int]
    PATH_FIELD_NUMBER: _ClassVar[int]
    modifyingFile: str
    owner: str
    path: str
    def __init__(self, path: _Optional[str] = ..., modifyingFile: _Optional[str] = ..., owner: _Optional[str] = ...) -> None: ...

class VerificationRequest(_message.Message):
    __slots__ = ["arguments", "code", "path", "timeout"]
    ARGUMENTS_FIELD_NUMBER: _ClassVar[int]
    CODE_FIELD_NUMBER: _ClassVar[int]
    PATH_FIELD_NUMBER: _ClassVar[int]
    TIMEOUT_FIELD_NUMBER: _ClassVar[int]
    arguments: _containers.RepeatedScalarFieldContainer[str]
    code: str
    path: str
    timeout: str
    def __init__(self, code: _Optional[str] = ..., path: _Optional[str] = ..., arguments: _Optional[_Iterable[str]] = ..., timeout: _Optional[str] = ...) -> None: ...

class VerificationResponse(_message.Message):
    __slots__ = ["executionTimeInMs", "fileName", "response"]
    EXECUTIONTIMEINMS_FIELD_NUMBER: _ClassVar[int]
    FILENAME_FIELD_NUMBER: _ClassVar[int]
    RESPONSE_FIELD_NUMBER: _ClassVar[int]
    executionTimeInMs: int
    fileName: str
    response: bytes
    def __init__(self, response: _Optional[bytes] = ..., fileName: _Optional[str] = ..., executionTimeInMs: _Optional[int] = ...) -> None: ...

class VerificationResponseList(_message.Message):
    __slots__ = ["executionTimeInMs", "responseList"]
    EXECUTIONTIMEINMS_FIELD_NUMBER: _ClassVar[int]
    RESPONSELIST_FIELD_NUMBER: _ClassVar[int]
    executionTimeInMs: int
    responseList: _containers.RepeatedCompositeFieldContainer[VerificationResponse]
    def __init__(self, responseList: _Optional[_Iterable[_Union[VerificationResponse, _Mapping]]] = ..., executionTimeInMs: _Optional[int] = ...) -> None: ...
