#pragma once
//===========================================================================

#ifndef _BONTUNER_20121029194141093_H_INCLUDED_
#define _BONTUNER_20121029194141093_H_INCLUDED_
//---------------------------------------------------------------------------

#include <string>
#include <vector>
#include <deque>
#include "IBonDriver2.h"

//---------------------------------------------------------------------------

struct VCHANNEL {
  BOOL Visible ;
  BOOL HasSignal ;
  BOOL HasStream ;
  std::wstring Name ;
  VCHANNEL(const std::wstring &Name_,BOOL Visible_=TRUE,BOOL HasSignal_=TRUE,BOOL HasStream_=TRUE)
    : Name(Name_), Visible(Visible_), HasSignal(HasSignal_), HasStream(HasStream_) {}
  VCHANNEL(const VCHANNEL &Src)
    : Name(Src.Name), Visible(Src.Visible), HasSignal(Src.HasSignal), HasStream(Src.HasStream) {}
  operator std::wstring() const {
    return Name ;
  }
};

typedef std::vector<VCHANNEL> VCHANNELS ;

struct VSPACE {
  std::wstring Name ;
  VCHANNELS Channels ;
  int MaxChannel ;
  BOOL Profiled ;
  BOOL Visible ;
  explicit VSPACE(std::wstring name) {
    Name = name ;
    MaxChannel = -1 ;
    Profiled = FALSE ;
    Visible = TRUE ;
  }
};

typedef std::vector<VSPACE> VSPACES ;

struct VTUNER {
  HINSTANCE Module ;
  IBonDriver2 *Tuner ;
  HANDLE Mutex ;
  VSPACES Spaces ;
  int MaxSpace ;
  BOOL Opening ;
  BOOL Profiled ;
  BOOL LastRotateFailed ;
  explicit VTUNER(HINSTANCE module,IBonDriver2 *tuner,HANDLE mutex=NULL) {
    Module = module ;
    Tuner = tuner ;
    Mutex = mutex ;
    MaxSpace = -1 ;
    Opening = FALSE ;
    Profiled = FALSE ;
    LastRotateFailed = FALSE ;
  }
  void Free() {
    if(Tuner) {
      if(Opening) {
        Tuner->CloseTuner() ;
        Opening = FALSE ;
      }
      Tuner->Release() ;
      Tuner=NULL ;
    }
    if(Module) {
      FreeLibrary(Module) ;
      Module=NULL ;
    }
    if(Mutex) {
      ReleaseMutex(Mutex) ;
      CloseHandle(Mutex) ;
      Mutex=NULL ;
    }
  }
};

typedef std::vector<VTUNER> VTUNERS ;

struct VSPACE_ANCHOR {
  int Tuner, Space ;
  explicit VSPACE_ANCHOR(int tuner, int spc) : Tuner(tuner), Space(spc) {}
};

typedef std::vector<VSPACE_ANCHOR> VSPACE_ANCHORS;

class CBonTuner : public IBonDriver2
{
private:
  exclusive_object  Exclusive ;

protected:
  int RefCount ;
  DWORD CurSpace, CurChannel ;
  DWORD CurRTuner, CurRSpace, CurRChannel ;
  BOOL CurHasSignal, CurHasStream ;
  std::wstring TunerName ;
  std::vector< std::deque<std::string> > TunerPaths ;
  std::vector< std::deque<HINSTANCE> > TunerModules ;
  VTUNERS Tuners ;
  VSPACE_ANCHORS SpaceAnchors ;
  std::string RecordTransitDir ;
  DWORD TunerRetryDuration ;
  BOOL FullLoad ;
  BOOL FullOpen ;
  BOOL FullScan ;
  BOOL LazyOpen ;
  BOOL ReloadOnTunerRetry ;
  BOOL CheckOpening ;
  BOOL CheckOpeningCloseOnFailure ;
  BOOL RecordTransit ;
  BOOL SaveCurrent ;
  BOOL OpenTunerOrdered ;
  BOOL ByteTuning ;
  BOOL ByteTuningCancelResult ;
  BOOL AvoidTunerMutex ;
  std::vector<std::wstring> TunerMutexAvoidPrefixNames ;
  BOOL ManageTunerMutex ;
  std::wstring TunerMutexPrefix ;
  std::vector<std::wstring> SpaceArrangeNames ;
  BOOL SpaceConcat ;
  std::wstring SpaceConcatName ;
  BOOL ChannelKeeping ;
  BOOL AsyncTSEnabled ;
  DWORD AsyncTSPacketSize ;
  DWORD AsyncTSQueueNum ;
  DWORD AsyncTSQueueMax ;
  DWORD AsyncTSQueueStart ;
  DWORD AsyncTSQueueCurStart ;
  DWORD AsyncTSEmptyBorder ;
  DWORD AsyncTSEmptyLimit ;
  BOOL AsyncTSModerateAllocating ;
  int AsyncTSRecvThreadPriority ;
  DWORD AsyncTSRecvThreadWait ;
  int AsyncTSFifoThreadPriority ;
  DWORD AsyncTSFifoThreadWait ;
  CAsyncFifo *AsyncTSFifo ;
  exclusive_object AsyncTSExclusive ;
  event_object *AsyncTSRecvEvent ;
  HANDLE AsyncTSRecvThread ;
  event_object *AsyncTSRecvDoneEvent ;
  BOOL AsyncTSTerminated ;
  BOOL MagicEnabled ;
  std::string MagicIPEndPoint ;
  WORD MagicDefaultPort ;
  std::string MagicMACAddress ;
  BOOL MagicBeforeTuning ;
  DWORD MagicRetry ;
  DWORD MagicSleep ;
  BOOL MagicSent ;
  BOOL SignalAsBitrate ;
  float SignalBitrate ;
  DWORD SignalBytesProceeded ;
  DWORD SignalTickProceeding ;
  BOOL MMTimerEnabled;
  UINT MMTimerPeriod;
  bool MMTimerCurEnabled;
  BOOL HRTimerEnabled;
  HANDLE HRTimerHandle;

protected:
  std::string MakeModuleFileName() ;
  std::string MakeModulePath() ;
  std::string MakeModulePrefix() ;
  BOOL VirtualToRealSpace(const DWORD dwSpace,DWORD &tuner,DWORD &spc) ;
  BOOL VirtualToRealChannel(const DWORD dwSpace,const DWORD dwChannel,DWORD &tuner,DWORD &spc,DWORD &ch) ;
  BOOL SerialToVirtualChannel(const DWORD SCh,DWORD &VSpace,DWORD &VCh) ;
  BOOL VirtualToSerialChannel(const DWORD VSpace,const DWORD VCh,DWORD &SCh) ;
  LPCTSTR EnumVirtualChannelName(const DWORD dwSpace, const DWORD dwChannel) ;
  BOOL SetVirtualChannel(const DWORD dwSpace, const DWORD dwChannel) ;
  void LoadIni() ;
  void SaveIni() ;
  void DoFullScan() ;
  void RotateTunerCandidates(size_t tuner) ;
  void FreeTunerModules() ;
  void PreloadTunerModules(bool nullAll=false) ;
  BOOL ReloadTunerModule(size_t tuner,bool forceReload=false) ;
  BOOL LoadTuner(size_t tuner,bool tuning=true,bool rotating=true) ;
  BOOL GetCurrentTunedChannel(DWORD &curSpace,DWORD &curChannel) ;
  BOOL ResetChannel() ;
  void DoChannelKeeping() ;
  void DoDropStream() ;
  void AsyncTSBegin() ;
  void AsyncTSEnd() ;
  unsigned int AsyncTSRecvThreadProcMain();
  static unsigned int __stdcall AsyncTSRecvThreadProc(LPVOID pv);
  BOOL SendMagic() ;

public:
  static CBonTuner *This ;
  static HINSTANCE Module ;

public: // contstructor / destructor
  CBonTuner();
  virtual ~CBonTuner();

public: // IBonDriver2 inherited methods
  virtual LPCTSTR GetTunerName(void) ;
  virtual const BOOL IsTunerOpening(void) ;
  virtual LPCTSTR EnumTuningSpace(const DWORD dwSpace) ;
  virtual LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel) ;
  virtual const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel) ;
  virtual const DWORD GetCurSpace(void) ;
  virtual const DWORD GetCurChannel(void) ;

public: // IBonDriver inherited methods
  virtual const BOOL OpenTuner(void) ;
  virtual void CloseTuner(void) ;
  virtual const BOOL SetChannel(const BYTE bCh) ;
  virtual const float GetSignalLevel(void) ;
  virtual const DWORD WaitTsStream(const DWORD dwTimeOut = 0) ;
  virtual const DWORD GetReadyCount(void) ;
  virtual const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain) ;
  virtual const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain) ;
  virtual void PurgeTsStream(void) ;
  virtual void Release(void) ;

public: // IBonDriver extra methods
  void AddRef() ;
  void Initialize() ;
} ;

//===========================================================================
#endif // _BONTUNER_20121029194141093_H_INCLUDED_
