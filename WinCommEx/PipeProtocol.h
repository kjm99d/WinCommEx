#pragma once
#include <Windows.h>

#pragma pack(push, 1)

typedef struct _PIPE_PROTOCOL
{
    DWORD cbData;
    DWORD dwType;
    BYTE payload[1];
} PIPE_PROTOCOL, * PPIPE_PROTOCOL;

#pragma pack(pop)


