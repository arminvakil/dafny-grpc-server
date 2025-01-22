from google.protobuf.internal import containers as _containers
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Iterable as _Iterable, Mapping as _Mapping, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class CreateDir(_message.Message):
    __slots__ = ("owner",)
    OWNER_FIELD_NUMBER: _ClassVar[int]
    owner: str
    def __init__(self, owner: _Optional[str] = ...) -> None: ...

class Empty(_message.Message):
    __slots__ = ()
    def __init__(self) -> None: ...

class CloneAndVerifyRequest(_message.Message):
    __slots__ = ("directoryPath", "code", "modifyingFile", "requestsList")
    DIRECTORYPATH_FIELD_NUMBER: _ClassVar[int]
    CODE_FIELD_NUMBER: _ClassVar[int]
    MODIFYINGFILE_FIELD_NUMBER: _ClassVar[int]
    REQUESTSLIST_FIELD_NUMBER: _ClassVar[int]
    directoryPath: str
    code: str
    modifyingFile: str
    requestsList: _containers.RepeatedCompositeFieldContainer[VerificationRequest]
    def __init__(self, directoryPath: _Optional[str] = ..., code: _Optional[str] = ..., modifyingFile: _Optional[str] = ..., requestsList: _Optional[_Iterable[_Union[VerificationRequest, _Mapping]]] = ...) -> None: ...

class TwoStageRequest(_message.Message):
    __slots__ = ("directoryPath", "runExclusive", "firstStageRequestsList", "prerequisiteForSecondStageRequestsList", "secondStageRequestsList")
    DIRECTORYPATH_FIELD_NUMBER: _ClassVar[int]
    RUNEXCLUSIVE_FIELD_NUMBER: _ClassVar[int]
    FIRSTSTAGEREQUESTSLIST_FIELD_NUMBER: _ClassVar[int]
    PREREQUISITEFORSECONDSTAGEREQUESTSLIST_FIELD_NUMBER: _ClassVar[int]
    SECONDSTAGEREQUESTSLIST_FIELD_NUMBER: _ClassVar[int]
    directoryPath: str
    runExclusive: bool
    firstStageRequestsList: _containers.RepeatedCompositeFieldContainer[VerificationRequest]
    prerequisiteForSecondStageRequestsList: _containers.RepeatedCompositeFieldContainer[VerificationRequest]
    secondStageRequestsList: _containers.RepeatedCompositeFieldContainer[VerificationRequest]
    def __init__(self, directoryPath: _Optional[str] = ..., runExclusive: bool = ..., firstStageRequestsList: _Optional[_Iterable[_Union[VerificationRequest, _Mapping]]] = ..., prerequisiteForSecondStageRequestsList: _Optional[_Iterable[_Union[VerificationRequest, _Mapping]]] = ..., secondStageRequestsList: _Optional[_Iterable[_Union[VerificationRequest, _Mapping]]] = ...) -> None: ...

class TmpFolder(_message.Message):
    __slots__ = ("path", "modifyingFile", "owner")
    PATH_FIELD_NUMBER: _ClassVar[int]
    MODIFYINGFILE_FIELD_NUMBER: _ClassVar[int]
    OWNER_FIELD_NUMBER: _ClassVar[int]
    path: str
    modifyingFile: str
    owner: str
    def __init__(self, path: _Optional[str] = ..., modifyingFile: _Optional[str] = ..., owner: _Optional[str] = ...) -> None: ...

class VerificationRequest(_message.Message):
    __slots__ = ("code", "path", "arguments", "timeout", "shouldPassNotFail")
    CODE_FIELD_NUMBER: _ClassVar[int]
    PATH_FIELD_NUMBER: _ClassVar[int]
    ARGUMENTS_FIELD_NUMBER: _ClassVar[int]
    TIMEOUT_FIELD_NUMBER: _ClassVar[int]
    SHOULDPASSNOTFAIL_FIELD_NUMBER: _ClassVar[int]
    code: str
    path: str
    arguments: _containers.RepeatedScalarFieldContainer[str]
    timeout: str
    shouldPassNotFail: bool
    def __init__(self, code: _Optional[str] = ..., path: _Optional[str] = ..., arguments: _Optional[_Iterable[str]] = ..., timeout: _Optional[str] = ..., shouldPassNotFail: bool = ...) -> None: ...

class VerificationResponseList(_message.Message):
    __slots__ = ("responseList", "executionTimeInMs")
    RESPONSELIST_FIELD_NUMBER: _ClassVar[int]
    EXECUTIONTIMEINMS_FIELD_NUMBER: _ClassVar[int]
    responseList: _containers.RepeatedCompositeFieldContainer[VerificationResponse]
    executionTimeInMs: int
    def __init__(self, responseList: _Optional[_Iterable[_Union[VerificationResponse, _Mapping]]] = ..., executionTimeInMs: _Optional[int] = ...) -> None: ...

class Z3OutputList(_message.Message):
    __slots__ = ("outputList", "executionTimeInMs")
    OUTPUTLIST_FIELD_NUMBER: _ClassVar[int]
    EXECUTIONTIMEINMS_FIELD_NUMBER: _ClassVar[int]
    outputList: _containers.RepeatedScalarFieldContainer[str]
    executionTimeInMs: int
    def __init__(self, outputList: _Optional[_Iterable[str]] = ..., executionTimeInMs: _Optional[int] = ...) -> None: ...

class VerificationResponse(_message.Message):
    __slots__ = ("response", "fileName", "executionTimeInMs", "exitStatus")
    RESPONSE_FIELD_NUMBER: _ClassVar[int]
    FILENAME_FIELD_NUMBER: _ClassVar[int]
    EXECUTIONTIMEINMS_FIELD_NUMBER: _ClassVar[int]
    EXITSTATUS_FIELD_NUMBER: _ClassVar[int]
    response: bytes
    fileName: str
    executionTimeInMs: int
    exitStatus: int
    def __init__(self, response: _Optional[bytes] = ..., fileName: _Optional[str] = ..., executionTimeInMs: _Optional[int] = ..., exitStatus: _Optional[int] = ...) -> None: ...
