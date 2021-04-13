#pragma once

#ifdef _DEBUG
#define DEBUG_TO_X_DRIVE
#endif

#include <stdio.h>
#include <stdarg.h>

	void __inline debug_printf( const char* format,... )
	{
		va_list marker ;
		va_start( marker, format ) ;
		int edit_ln = _vscprintf(format, marker);
		if(edit_ln++>0) {
			char *edit_str = static_cast<char*>(alloca(edit_ln)) ;
			vsprintf_s( edit_str, edit_ln, format, marker ) ;
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
		va_end( marker ) ;
	}

#ifdef _DEBUG
#define DBG_INFO(...) do{ char d_buff[128]; \
	_snprintf_s(d_buff,128, __VA_ARGS__); \
	::OutputDebugStringA(d_buff);}while(0)
#define TRACE(...) do{ wchar_t d_buff[128]; \
	_snwprintf_s(d_buff,128, __VA_ARGS__); \
	::OutputDebugStringW(d_buff);}while(0)
	#define DBGOUT(...) debug_printf(__VA_ARGS__)
	#define LINEDEBUG DBGOUT("%s(%d): passed.\n",__FILE__,__LINE__)
#else
#define DBG_INFO(...) /*empty*/
#define TRACE(...) /*empty*/
#define DBGOUT(...) /*empty*/
#define LINEDEBUG /*empty*/
#endif

#define ERROUT(...) do { \
		debug_printf("%s(%d): ",__FILE__,__LINE__);\
		debug_printf(__VA_ARGS__); \
	}while(0)
