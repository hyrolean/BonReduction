// SpinelRD.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "Daemon.h"
#include "SpinelRD.h"

#define MAX_LOADSTRING 100

BOOL QuitOrder = FALSE ;

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR    lpCmdLine,
					 int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SPINELRD, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	nCmdShow = SW_HIDE ; // バックグラウンド動作の為、ウィンドウを隠しておく

	// アプリケーションの初期化を実行します:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPINELRD));

	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if(QuitOrder) {
				PostQuitMessage(0);
				QuitOrder=FALSE;
			}
		}
	}

	MainDaemon.Finalize();

	return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
//  コメント:
//
//    この関数および使い方は、'RegisterClassEx' 関数が追加された
//    Windows 95 より前の Win32 システムと互換させる場合にのみ必要です。
//    アプリケーションが、関連付けられた
//    正しい形式の小さいアイコンを取得できるようにするには、
//    この関数を呼び出してください。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPINELRD));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SPINELRD);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
	  return FALSE;
   }

   //MainDaemon.Initialize(hInstance,hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void DoAboutDialog(HWND hWnd)
{
  if(DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About)==IDABORT)
	QuitOrder = TRUE ;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL defProc=FALSE ;
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_ABOUT:
			DoAboutDialog(hWnd);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			defProc = TRUE;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;
	case WM_CREATE:
		MainDaemon.Initialize(hInst,hWnd);
		defProc = TRUE;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_POWERBROADCAST:
		MainDaemon.WMPowerBroadCast(wParam,lParam);
		defProc = TRUE;
		break;
	case WM_ENDSESSION:
		MainDaemon.WMEndSession(wParam,lParam);
		defProc = TRUE;
		break;
	case WM_USERTASKBAR:
		MainDaemon.WMUserTaskbar(wParam,lParam);
		//defProc = TRUE;
		break;
	default:
		defProc = TRUE;
	}
	if(defProc)
	  return DefWindowProc(hWnd, message, wParam, lParam);
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SendMessage(hDlg,WM_SETICON,ICON_SMALL,
			(LPARAM)LoadImage(hInst, MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON, 16, 16, 0));
		SetWindowText(GetDlgItem(hDlg,IDC_STATIC_VER),_T(SPINELRD_VER));
		SendMessage(GetDlgItem(hDlg,IDC_CHECK_JOBPAUSE), BM_SETCHECK,
			MainDaemon.DAEMONJobPaused()?BST_CHECKED:BST_UNCHECKED, 0);
		SendMessage(GetDlgItem(hDlg,IDC_CHECK_DEATHRESUME), BM_SETCHECK,
			MainDaemon.SpinelDeathResumeEnabled()?BST_CHECKED:BST_UNCHECKED, 0);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDABORT)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		switch(LOWORD(wParam)) {
			case IDC_CHECK_JOBPAUSE:
				MainDaemon.DAEMONPauseJob(
					BST_CHECKED==SendMessage(HWND(lParam), BM_GETCHECK, 0, 0)?
					TRUE:FALSE );
				break;
			case IDC_CHECK_DEATHRESUME:
				MainDaemon.SpinelEnableDeathResume(
					BST_CHECKED==SendMessage(HWND(lParam), BM_GETCHECK, 0, 0)?
					TRUE:FALSE );
				break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
