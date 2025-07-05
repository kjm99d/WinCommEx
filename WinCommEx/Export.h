#pragma once

#if _DLL	// DLL 빌드 인 경우
	#ifdef WINCOMMEX_EXPORTS
		#define WINCOMMEX_API __declspec(dllexport)
	#else
		#define WINCOMMEX_API __declspec(dllimport)
	#endif

#else		// DLL 빌드가 아닌 경우

#endif

