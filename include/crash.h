#pragma once

#include <windows.h>

LONG WINAPI unhandledExceptionFilter(_In_ struct _EXCEPTION_POINTERS*);
LONG NTAPI uncatchableExceptionHandler(struct _EXCEPTION_POINTERS*);
