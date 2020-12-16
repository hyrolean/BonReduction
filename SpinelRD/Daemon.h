//===========================================================================
#pragma once
//---------------------------------------------------------------------------

#include <map>
#include <string>
#include <vector>
#include <shellApi.h>
//===========================================================================

  // �Ď��^�C�}�[�N���X CDaemonTimer
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

  struct TRotationItem {
    std::string FileMask;
    size_t MaxFiles;
    int MaxDays;
    __int64 MaxBytes;
    std::vector<std::string> FellowSuffix ;
    TRotationItem(std::string FileMask_,int MaxFiles_,int MaxDays_,
      __int64 MaxBytes_,const std::vector<std::string> &FellowSuffix_=std::vector<std::string>())
      : FileMask(FileMask_),MaxFiles(MaxFiles_),MaxDays(MaxDays_),MaxBytes(MaxBytes_),FellowSuffix(FellowSuffix_)
    {}
    TRotationItem(const TRotationItem &src)
      : FileMask(src.FileMask),MaxFiles(src.MaxFiles),MaxDays(src.MaxDays),MaxBytes(src.MaxBytes),FellowSuffix(src.FellowSuffix)
    {}
  };
  typedef std::vector<TRotationItem> TRotations;

  // ���C���Ď��N���X CMainDaemon
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
protected:
  bool Suspended, Resumed, EndSession, JobAborted ;
  BOOL DAEMONShowTaskIcon ;
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
protected: // �R�[���o�b�N
  void OnStartTimer();
  void OnResumeTimer();
  void OnJobTimer();
  void DaemonTimerEventMain(CDaemonTimer *Timer);
  static void DaemonTimerEventCallback(CDaemonTimer *Timer);
  static unsigned int __stdcall JobThreadProc(PVOID pv) ;
public: // �E�B���h�E���b�Z�[�W����
  void WMPowerBroadCast(WPARAM wParam, LPARAM lParam) ;
  void WMEndSession(WPARAM wParam, LPARAM lParam) ;
  void WMUserTaskbar(WPARAM wParam, LPARAM lParam) ;
public:
  CMainDaemon();
  void Initialize(HINSTANCE InstanceHandle, HWND MainWindowHandle);
  void Finalize();
};

extern CMainDaemon MainDaemon;

//===========================================================================