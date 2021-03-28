//===========================================================================
#pragma once
//---------------------------------------------------------------------------

#include <map>
#include <string>
#include <vector>
#include <shellApi.h>
//===========================================================================

  // 監視タイマークラス CDaemonTimer
class CDaemonTimer
{
public:
  typedef void (*EVENT_DaemonTimer)(CDaemonTimer *Timer) ;
protected:
  HWND hwnd_;
  bool enabled_;
  UINT id_;
  int interval_;
  EVENT_DaemonTimer event_;
  void *arg_;
protected:
  static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
  void Update() ;
public:
  CDaemonTimer(HWND hwnd, EVENT_DaemonTimer event, void *arg=NULL, int interval=1000, bool enabled=true, int id=0);
  virtual ~CDaemonTimer() ;
  HWND WindowHandle() { return hwnd_; }
  bool Enabled() {return enabled_; }
  void SetEnabled(bool val) { if(enabled_!=val) { enabled_=val; Update(); } }
  UINT Id() { return id_; }
  void *Arg() { return arg_; }
  int Interval() { return interval_; }
  void SetInterval(int val) { if(interval_!=val) { interval_=val; Update(); } }
  void DoEvent() { if(event_) event_(this); }
};

typedef std::map<int,CDaemonTimer*> MAP_DaemonTimer ;


//---------------------------------------------------------------------------

  typedef std::vector<std::string> masks_t;

  struct TRotationItem {
    std::string FilePath;
    masks_t Masks;
    size_t MaxFiles;
    int MaxDays;
    __int64 MaxBytes;
    bool SubDirectories;
    masks_t FellowMasks ;
    TRotationItem(std::string FileMasks_,int MaxFiles_,int MaxDays_,
      __int64 MaxBytes_,bool SubDirectories_=false,std::string FellowSuffix_="^")
      : MaxFiles(MaxFiles_),MaxDays(MaxDays_),
        MaxBytes(MaxBytes_),SubDirectories(SubDirectories_) {
      FilePath = file_path_of(FileMasks_) ;
      std::string MaskCSV = file_name_of(FileMasks_) ;
      if(MaskCSV!="^") {
        split(Masks,MaskCSV,',');
      }
      if(FellowSuffix_!="^") {
        split(FellowMasks,FellowSuffix_,',');
      }
    }
  };
  typedef std::vector<TRotationItem> TRotations;

  // メイン監視クラス CMainDaemon
class CMainDaemon
{
private:
  HINSTANCE HInstance;
  HWND HWMain;
  bool first, niEntried;
  NOTIFYICONDATA niData;
  HANDLE JobThread ;
  CDaemonTimer *StartTimer;
  CDaemonTimer *ResumeTimer;
  CDaemonTimer *JobTimer;
  std::string AppExeName();
  exclusive_object exclLog;
protected:
  bool Suspended, Resumed, EndSession, JobAborted ;
  BOOL DAEMONShowTaskIcon ;
  BOOL DAEMONLogEnabled ;
  BOOL DAEMONJobPause ;
  int DAEMONThreadPriority ;
  BOOL SpinelEnabled ;
  std::string SpinelExeName ;
  std::string SpinelExeParam ;
  BOOL SpinelDeathResume ;
  DWORD SpinelProcessPriority ;
  HANDLE SpinelProcess ;
  BOOL SpinelRemoveUISetting ;
  BOOL LogRotationEnabled ;
  int LogRotationMaxFiles ;
  __int64 LogRotationMaxBytes ;
  TRotations UserRotations, JobRotations ;
  int IniLastAge ;
  void LoadIni() ;
  void RemoveUISetting() ;
  bool LaunchSpinel() ;
  void JobRotate() ;
  void JobAll() ;
  void AddTaskIcon() ;
  void DeleteTaskIcon() ;
  bool TerminateJob(bool checkOnly=false) ;
protected: // コールバック
  void OnStartTimer();
  void OnResumeTimer();
  void OnJobTimer();
  void DaemonTimerEventMain(CDaemonTimer *Timer);
  static void DaemonTimerEventCallback(CDaemonTimer *Timer);
  static unsigned int __stdcall JobThreadProc(PVOID pv) ;
public: // ウィンドウメッセージ処理
  void WMPowerBroadCast(WPARAM wParam, LPARAM lParam) ;
  void WMEndSession(WPARAM wParam, LPARAM lParam) ;
  void WMUserTaskbar(WPARAM wParam, LPARAM lParam) ;
public:
  CMainDaemon();
  void Initialize(HINSTANCE InstanceHandle, HWND MainWindowHandle);
  void Finalize();
  void LogOut(const char* format, ...) ;
};

extern CMainDaemon MainDaemon;

//===========================================================================
