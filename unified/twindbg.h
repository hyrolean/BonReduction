#pragma once

#define DEBUG_TO_X_DRIVE

#ifdef _DEBUG
#include <stdio.h>
#include <stdarg.h>
#define DBG_INFO(...) do{ char d_buff[128]; \
	_snprintf_s(d_buff,128, __VA_ARGS__); \
	::OutputDebugStringA(d_buff);}while(0)
#define TRACE(...) do{ wchar_t d_buff[128]; \
	_snwprintf_s(d_buff,128, __VA_ARGS__); \
	::OutputDebugStringW(d_buff);}while(0)
    void __inline DBGOUT( const char* format,... )
    {
        va_list marker ;
        char edit_str[1024] ;
        va_start( marker, format ) ;
        vsprintf_s( edit_str, sizeof(edit_str), format, marker ) ;
        va_end( marker ) ;
        #ifndef DEBUG_TO_X_DRIVE
        OutputDebugStringA(edit_str) ;
        #else
        {
			FILE *fp = NULL ;
            fopen_s(&fp,"X:\\Debug.txt","a+t") ;
            if(fp) {
              fputs(edit_str,fp) ;
              fclose(fp) ;
            }
		}
        #endif
    }
#else
#define DBG_INFO(...) /*empty*/
#define TRACE(...) /*empty*/
#define DBGOUT(...) /*empty*/
#endif

