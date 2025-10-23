#pragma once

#ifdef WINCOMMEX_EXPORTS
#define WINCOMMEX_API __declspec(dllexport)
#else
#define WINCOMMEX_API __declspec(dllimport)
#endif



