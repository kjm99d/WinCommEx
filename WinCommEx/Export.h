#pragma once

#if _DLL	// DLL ���� �� ���
	#ifdef WINCOMMEX_EXPORTS
		#define WINCOMMEX_API __declspec(dllexport)
	#else
		#define WINCOMMEX_API __declspec(dllimport)
	#endif

#else		// DLL ���尡 �ƴ� ���

#endif

