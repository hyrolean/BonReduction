// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "stdafx.h"
#include "BonTuner.h"

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch(ul_reason_for_call){
		case DLL_PROCESS_ATTACH:

#ifdef _DEBUG
			// メモリリーク検出有効
			::_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
#endif

			// モジュールハンドル保存
			CBonTuner::Module = hModule;
			break;

		case DLL_PROCESS_DETACH:
			// 未開放の場合はインスタンス開放
			if(CBonTuner::This)
			  CBonTuner::This->Release();
			break;
		}

    return TRUE;
}
