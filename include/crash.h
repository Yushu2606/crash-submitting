#pragma once

#include <windows.h>

LONG NTAPI uncatchableExceptionHandler(_In_ struct _EXCEPTION_POINTERS*);
LONG NTAPI unhandledExceptionFilter(_In_ struct _EXCEPTION_POINTERS*);
