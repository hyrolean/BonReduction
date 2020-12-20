//===========================================================================
#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <assert.h>
#include <SetupApi.h>
#include <Malloc.h>
#include <InitGuid.h>
#include <Math.h>
#include <cstdio>
#include <io.h>
#include <map>
#include <iterator>
#include <algorithm>
#include <process.h>
#include <winsock2.h>
#include "Resource.h"
#include "BonTuner.h"
//---------------------------------------------------------------------------
#pragma comment(lib, "WSock32.Lib")

using namespace std ;

#define AsyncTSSuspend_() do { \
      DBGOUT("AsyncTS is suspending at line: %d.\r\n",__LINE__); \
      if(OpenTunerOrdered) AsyncTSEnd(); \
    }while(0)

#define AsyncTSResume_() do { \
      if(OpenTunerOrdered) AsyncTSBegin(); \
      DBGOUT("AsyncTS was resumed at line: %d.\r\n",__LINE__); \
    }while(0)

//===========================================================================
  // DLL exports
//---------------------------------------------------------------------------

#pragma warning( disable : 4273 )

typedef IBonDriver *(*CREATEBONDRIVER)() ;

extern "C" __declspec(dllexport) IBonDriver * CreateBonDriver()
{
    if(CBonTuner::This)
      CBonTuner::This->AddRef() ;
    else {
      new CBonTuner ;
      if(CBonTuner::This)
        CBonTuner::This->Initialize() ;
    }

    return CBonTuner::This ;
}

#pragma warning( default : 4273 )

//===========================================================================
  // Classes
//---------------------------------------------------------------------------

  // CTransitBase
class CTransitBase
{
protected:
  typedef map<string/*name*/,string/*value*/> TRANSIT ;
  TRANSIT Transit ;
  string FileName ;
protected:
  explicit CTransitBase(const string &_FileName): Transit() {
    FileName = _FileName ;
  }
  virtual ~CTransitBase() {}
};

  // CTransitReader
class CTransitReader : public CTransitBase
{
private:
  void Parse(const string &line) {
    string::size_type eq_idx = line.find("=") ;
    if(eq_idx==string::npos) return ;
    Transit[trim(line.substr(0,eq_idx))]=trim(line.substr(eq_idx+1)) ;
  }
public:
  explicit CTransitReader(const string &_FileName)
    : CTransitBase(_FileName) {
    Reload() ;
  }
  virtual ~CTransitReader() {}
  string ReadString(const string &name,const string &defVal="") const {
    TRANSIT::const_iterator pos = Transit.find(name) ;
    if(pos==Transit.end())
      return defVal ;
    return pos->second ;
  }
  int ReadInteger(const string &name,int defDig=0) const {
    TRANSIT::const_iterator pos = Transit.find(name) ;
    if(pos==Transit.end())
      return defDig ;
    return atoi(pos->second.c_str()) ;
  }
  BOOL Reload() {
    Transit.clear() ;
    FILE *st = NULL ;
    fopen_s(&st,FileName.c_str(),"rt") ;
    if(!st) return FALSE ;
    string line ;
    while(!feof(st)) {
      int c=getc(st) ;
      if(c=='\n'||c==EOF) {
        Parse(line) ;
        line.clear() ;
      }else line += (char) c ;
    }
    if(line.length()>0)
      Parse(line) ;
    fclose(st) ;
    return TRUE ;
  }
};

  // CTransitProfiler
class CTransitProfiler : public CTransitReader
{
private:
  bool Modified ;
  bool Appended ;
  typedef vector< pair<string/*name*/,string/*value*/> > NEWTRANSIT ;
  NEWTRANSIT NewTransit ;
  template<class iterator> bool Describe(const string &mode,iterator beg,iterator end) {
    FILE *st = NULL ;
    fopen_s(&st,FileName.c_str(),mode.c_str()) ;
    if(!st) return false ;
    for(iterator pos = beg ; pos!=end ; ++pos) {
      fprintf(st,"%s=%s\n",pos->first.c_str(),pos->second.c_str()) ;
    }
    fclose(st) ;
    return true ;
  }
  #ifdef _DEBUG
  void debug(const string &text) {
    FILE *st=NULL ;
    fopen_s(&st,(FileName+".debug.txt").c_str(),"a+t") ;
    if(st) {
      fprintf(st,"%s\n",text.c_str());
      fclose(st) ;
    }
  }
  #endif
public:
  explicit CTransitProfiler(const string &_FileName)
    : CTransitReader(_FileName),Modified(false),Appended(false),NewTransit()
  {}
  virtual ~CTransitProfiler() {
    Flush() ;
  }
  void WriteString(const string &name, const string &value) {
    TRANSIT::iterator pos = Transit.find(name) ;
    if(pos==Transit.end()) {
      #ifdef _DEBUG
      debug("Appended: "+name+" = "+value) ;
      #endif
      Transit[name] = value ;
      NewTransit.push_back(make_pair(name,value)) ;
      Appended = true ;
    }
    else if(pos->second!=value) {
      #ifdef _DEBUG
      debug("Modified: "+name+" = "+pos->second+" -> "+value) ;
      #endif
      pos->second = value ;
      Modified = true ;
    }
  }
  void WriteInteger(const string &name, int digits) {
    WriteString(name,itos(digits)) ;
  }
  BOOL Flush(bool Force=false) {
    if(!Modified&&!Appended&&!Force) return TRUE ;
    if(Force||Modified) {
      if(!Describe("wt",Transit.begin(),Transit.end())) return FALSE ;
    }else /*Appended*/ {
      if(!Describe("a+t",NewTransit.begin(),NewTransit.end())) return FALSE ;
    }
    NewTransit.clear() ;
    Modified = false ;
    Appended = false ;
    return TRUE ;
  }
};


  // CMagicClient
class CMagicClient
{
protected:
  SOCKET Sock ;
public:
  CMagicClient() {
    Sock = NULL;
    WSAData wsaData;
    WSAStartup(MAKEWORD(2,0), &wsaData);
  }
  ~CMagicClient() {
    Close();
    WSACleanup();
  }
  BOOL Open() {
    if( Sock != NULL ){
        return FALSE;
    }
    Sock = socket(AF_INET, SOCK_DGRAM, 0);
    if( Sock == INVALID_SOCKET ){
        Sock = NULL;
        return FALSE;
    }
    BYTE yes=1 ;
    if(setsockopt(Sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes) )!=0) {
        return FALSE ;
    }
    return TRUE;
  }
  BOOL Close() {
    if( Sock != NULL ){
        closesocket(Sock);
        Sock = NULL;
    }
    return TRUE;
  }
  BOOL Send(string ip, WORD port, BYTE mac[6])
  {
    BYTE buffer[6+6*16] ;
    for(int i=0; i<6; i++) buffer[i]=0xFF ;
    for(int i=0,n=6; i<16; i++)
      for(int j=0; j<6; j++,n++) buffer[n] = mac[j] ;
    struct sockaddr_in addr ;
    ZeroMemory(&addr,sizeof(addr)) ;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((WORD)port);
    addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    return sendto(Sock,(char*)buffer,6+6*16,
         0, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ? FALSE:TRUE ;
  }
};


//===========================================================================
  // CBonTuner
//---------------------------------------------------------------------------
CBonTuner * CBonTuner::This = NULL;
HINSTANCE CBonTuner::Module = NULL;
#define TRANSIT_SUFFIX ".transit.txt"
//-----
CBonTuner::CBonTuner()
{
  assert(This==NULL) ;
  DBGOUT("----- init -----\r\n") ;
  This = this ;
  RefCount=1 ;
  OpenTunerOrdered = FALSE ;
  CurSpace=0 ;
  CurChannel=0 ;
  CurRTuner=0 ;
  CurRSpace=0 ;
  CurHasSignal=0 ;
  CurHasStream=0 ;
  TunerRetryDuration=1000 ;
  ReloadOnTunerRetry=1 ;
  FullLoad=0 ;
  FullOpen=0 ;
  FullScan=1 ;
  LazyOpen=1 ;
  CheckOpening=1 ;
  CheckOpeningCloseOnFailure=1 ;
  ByteTuning=1 ;
  ByteTuningCancelResult=FALSE ;
  RecordTransit=1 ;
  RecordTransitDir = "" ;
  SaveCurrent=1 ;
  ManageTunerMutex=0 ;
  SpaceConcat=0 ;
  SpaceConcatName= L"連結空間" ;
  ChannelKeeping=0 ;
  AsyncTSEnabled=FALSE ;
  AsyncTSPacketSize = 131072 ;
  AsyncTSQueueNum = 20 ;
  AsyncTSQueueMax = 100 ;
  AsyncTSEmptyBorder = 10 ;
  AsyncTSEmptyLimit = 5 ;
  AsyncTSModerateAllocating = TRUE ;
  AsyncTSRecvThreadWait = 50 ;
  AsyncTSRecvThreadPriority=THREAD_PRIORITY_HIGHEST ;
  AsyncTSFifo=NULL ;
  AsyncTSFifoThreadWait = 1000 ;
  AsyncTSFifoThreadPriority=THREAD_PRIORITY_HIGHEST ;
  AsyncTSRecvEvent=NULL ;
  AsyncTSRecvThread=NULL ;
  AsyncTSRecvDoneEvent=NULL ;
  AsyncTSTerminated=FALSE ;
  MagicEnabled=FALSE ;
  MagicIPEndPoint="255.255.255.255" ;
  MagicDefaultPort=9 ;
  MagicMACAddress="00:00:00:00:00:00" ;
  MagicRetry=10 ;
  MagicSleep=0 ;
  MagicSent=FALSE ;
  SignalAsBitrate=0 ;
  SignalBitrate=0.f ;
  SignalBytesProceeded=0 ;
  SignalTickProceeding=0 ;
}
//-----
CBonTuner::~CBonTuner()
{
  CloseTuner() ;
  FreeTunerModules() ;
  assert(This!=NULL) ;
  assert(RefCount==1) ;
  This = NULL ;
  DBGOUT("----- term -----\r\n") ;
}
//-----
void CBonTuner::Initialize()
{
  LoadIni();
  if(FullLoad) PreloadTunerModules(!!LazyOpen);
}
//-----
string CBonTuner::MakeModuleFileName()
{
  char szPath[_MAX_PATH] ;
  GetModuleFileNameA( Module, szPath, _MAX_PATH ) ;
  return szPath ;
}
//-----
string CBonTuner::MakeModulePath()
{
  return file_path_of(MakeModuleFileName()) ;
}
//-----
string CBonTuner::MakeModulePrefix()
{
  return file_prefix_of(MakeModuleFileName()) ;
}
//-----
void CBonTuner::LoadIni()
{
  string modPath = MakeModulePath() ;
  string modPrefix = MakeModulePrefix() ;
  string iniFileName = modPath+modPrefix+".ini" ;
  string transitFileName = modPath+modPrefix+TRANSIT_SUFFIX ;

  const DWORD BUFFER_SIZE = 1024 ;
  char buffer[BUFFER_SIZE] ;
  ZeroMemory(buffer,BUFFER_SIZE) ;


  #define LOADSTR2(val,key) do { \
      GetPrivateProfileStringA(Section,key,val.c_str(),buffer,BUFFER_SIZE,iniFileName.c_str()) ; \
      val = buffer ; \
    }while(0)
  #define LOADSTR(val) LOADSTR2(val,#val)
  #define LOADWSTR(val) do { \
      string temp = wcs2mbcs(val) ; \
      LOADSTR2(temp,#val) ; val = mbcs2wcs(temp) ; \
    }while(0)

  #define LOADINT2(val,key,a2i) do { \
      GetPrivateProfileStringA(Section,key,"",buffer,BUFFER_SIZE,iniFileName.c_str()) ; \
      val = a2i(buffer,val) ; \
    }while(0)
  #define LOADINT(val) LOADINT2(val,#val,acalci)
  #define LOADINT64(val) LOADINT2(val,#val,acalci64)

  {
    const char *Section = "Setting" ;
    // Transit ファイルに遷移状態を記録するかどうか
    LOADINT(RecordTransit) ;
    if(!RecordTransit) FullScan=0 ;
    // Transit 出力ディレクトリ
    LOADSTR(RecordTransitDir) ;
    if(!RecordTransitDir.empty()) {
      if(RecordTransitDir[RecordTransitDir.length()-1]!='\\')
        RecordTransitDir += '\\' ;
      transitFileName = RecordTransitDir+modPrefix+TRANSIT_SUFFIX ;
    }
    // チューナーオープンに費やす最大再試行時間
    LOADINT(TunerRetryDuration) ;
    // チューナーモジュールを事前に読み出して準備しておくかどうか
    LOADINT(FullLoad) ;
    if(FullLoad) ReloadOnTunerRetry = LazyOpen = 0 ;
    // チューナーオープンに失敗した場合にモジュールをロードし直すかどうか
    LOADINT(ReloadOnTunerRetry) ;
    // 一度開いたチューナーを開きっぱなしにするかどうか
    LOADINT(FullOpen) ;
    // チューニングしたときについでにチャンネル情報をフルスキャンするかどうか
    LOADINT(FullScan) ;
    // 実際に使用する直前までチューナーを開いたことにして済ますかどうか
    LOADINT(LazyOpen) ;
    // チューナーが開いている状態であるかどうか厳密に検査するかどうか
    LOADINT(CheckOpening) ;
    LOADINT(CheckOpeningCloseOnFailure);
    // バイトサイズのチャンネル変更方式を有効にするかどうか
    LOADINT(ByteTuning) ;
    LOADINT(ByteTuningCancelResult) ;
    // 現在のチャンネルチューニング状態を保存するかどうか
    LOADINT(SaveCurrent) ;
    // チューナー読み出し
    LOADWSTR(TunerName) ;
    for(int i=0;;i++) {
      string TunerPath ;
      string key = "TunerPath"+itos(i) ;
      LOADSTR2(TunerPath,key.c_str()) ;
      if(TunerPath.empty()) break ;
      deque<string> PathList ;
      split(PathList,TunerPath,';') ;
      TunerPaths.push_back(PathList) ;
    }
    if(TunerPaths.empty()) {
      string TunerPath ;
      LOADSTR(TunerPath) ;
      if(!TunerPath.empty()) {
        deque<string> PathList ;
        split(PathList,TunerPath,';') ;
        TunerPaths.push_back(PathList) ;
      }
    }
    //チューナーミューテックスの管理
    LOADINT(ManageTunerMutex) ;
    //スペース並替
    std::wstring SpaceArrangement=L"";
    LOADWSTR(SpaceArrangement) ;
    split(SpaceArrangeNames,SpaceArrangement,',');
    if(!SpaceArrangeNames.empty()) FullScan=1; //※フルスキャン強制
    //連結スペース
    LOADINT(SpaceConcat) ;
    LOADWSTR(SpaceConcatName) ;
    //チャンネル状態の保持
    LOADINT(ChannelKeeping) ;
    //非同期TS
    LOADINT(AsyncTSEnabled) ;
    LOADINT(AsyncTSPacketSize) ;
    LOADINT(AsyncTSQueueNum) ;
    LOADINT(AsyncTSQueueMax) ;
    LOADINT(AsyncTSEmptyBorder) ;
    LOADINT(AsyncTSEmptyLimit) ;
    LOADINT(AsyncTSModerateAllocating) ;
    LOADINT(AsyncTSRecvThreadPriority) ;
    LOADINT(AsyncTSRecvThreadWait) ;
    LOADINT(AsyncTSFifoThreadPriority) ;
    LOADINT(AsyncTSFifoThreadWait) ;
    //マジックパケット
    LOADINT(MagicEnabled) ;
    LOADSTR(MagicIPEndPoint) ;
    LOADINT(MagicDefaultPort) ;
    LOADSTR(MagicMACAddress) ;
    LOADINT(MagicRetry) ;
    LOADINT(MagicSleep) ;
    //シグナルビットレート
    LOADINT(SignalAsBitrate) ;
  }

  if(SaveCurrent) {
    const char *Section = "Current" ;
    LOADINT(CurSpace) ;
    LOADINT(CurChannel) ;
    LOADINT(CurRTuner) ;
    LOADINT(CurRSpace) ;
  }

  if(RecordTransit) {
    CTransitReader reader(transitFileName) ;
    for(size_t i=0;i<TunerPaths.size();i++) {
      string tuner_key = "Tuner"+itos(static_cast<int>(i)) ;
      int MaxSpace = reader.ReadInteger(tuner_key+".MaxSpace",-1) ;
      VTUNER Tuner(NULL,NULL) ;
      for(int j=0;j<MaxSpace;j++) {
        string tuner_space_key = tuner_key + ".Space" + itos(j) ;
        string SpaceName = reader.ReadString(tuner_space_key) ;
        int MaxChannel = reader.ReadInteger(tuner_space_key + ".MaxChannel",-1) ;
        BOOL Visible = reader.ReadInteger(tuner_space_key + ".Visible",TRUE) ;
        VSPACE Space(mbcs2wcs(SpaceName)) ;
        for(int k=0;k<MaxChannel;k++) {
          std::string chName = tuner_space_key + ".Channel" + itos(k) ;
          Space.Channels.push_back( VCHANNEL(
            mbcs2wcs(reader.ReadString(chName)),
            reader.ReadInteger(chName+".HasSignal",1)?TRUE:FALSE,
            reader.ReadInteger(chName+".HasStream",1)?TRUE:FALSE ) ) ;
        }
        Space.MaxChannel = MaxChannel ;
        if(MaxChannel>=0) Space.Profiled = TRUE ;
        Space.Visible = Visible ;
        Tuner.Spaces.push_back(Space) ;
      }
      if(MaxSpace<0) break ;
      Tuner.MaxSpace = MaxSpace ;
      Tuner.Profiled = TRUE ;
      Tuners.push_back(Tuner) ;
    }
  }

  #undef LOADINT64
  #undef LOADINT
  #undef LOADINT2

  #undef LOADSTR
  #undef LOADWSTR
  #undef LOADSTR2
}
//-----
void CBonTuner::SaveIni()
{
  string modPath = MakeModulePath() ;
  string modPrefix = MakeModulePrefix() ;
  string iniFileName = modPath+modPrefix+".ini" ;
  string transitFileName = modPath+modPrefix+TRANSIT_SUFFIX ;

  if(!RecordTransitDir.empty()) {
    transitFileName = RecordTransitDir+modPrefix+TRANSIT_SUFFIX ;
  }

  #define SAVESTR2(val,key) WritePrivateProfileStringA(Section,key,val.c_str(),iniFileName.c_str())
  #define SAVESTR(val) SAVESTR2(val,#val)

  #define SAVEINT2(val,key) SAVESTR2(itos(val),key)
  #define SAVEINT(val) SAVEINT2(val,#val)

  if(SaveCurrent) {
    const char *Section = "Current" ;
    SAVEINT(CurSpace) ;
    SAVEINT(CurChannel) ;
    SAVEINT(CurRTuner) ;
    SAVEINT(CurRSpace) ;
  }

  if(RecordTransit) {
    CTransitProfiler profiler(transitFileName) ;
    for(size_t i=0;i<Tuners.size();i++) {
      string tuner_key = "Tuner" + itos(static_cast<int>(i)) ;
      int MaxSpace = Tuners[i].MaxSpace ;
      if(MaxSpace<0) break ;
      if(!Tuners[i].Profiled) {
        profiler.WriteInteger(tuner_key + ".MaxSpace", MaxSpace) ;
        Tuners[i].Profiled = TRUE ;
      }
      for(int j=0;j<MaxSpace;j++) {
        if(Tuners[i].Spaces[j].Profiled)
          continue ;
        int MaxChannel = Tuners[i].Spaces[j].MaxChannel ;
        string tuner_space_key = tuner_key + ".Space" + itos(j) ;
        profiler.WriteString(tuner_space_key, wcs2mbcs(Tuners[i].Spaces[j].Name) );
        profiler.WriteInteger(tuner_space_key + ".Visible", Tuners[i].Spaces[j].Visible) ;
        if(MaxChannel<0) continue ;
        profiler.WriteInteger(tuner_space_key + ".MaxChannel", MaxChannel) ;
        for(int k=0;k<MaxChannel;k++) {
          profiler.WriteString(tuner_space_key + ".Channel" + itos(k),
            wcs2mbcs(Tuners[i].Spaces[j].Channels[k]));
        }
        Tuners[i].Spaces[j].Profiled = TRUE ;
      }
    }
  }

  #undef SAVEINT
  #undef SAVEINT2

  #undef SAVESTR
  #undef SAVESTR2
}
//-----
void CBonTuner::DoFullScan()
{
  //チャンネルのフルスキャン
  if(FullScan) {
    if(SpaceAnchors.empty()) {
      exclusive_lock lock(&Exclusive) ;
      // スペースアンカー構築
      struct space_t {
        VSPACE_ANCHOR anchor ;
        wstring name ;
        explicit space_t(int tun, int spc, wstring nam)
         : anchor(tun,spc), name(nam) {}
      };
      struct space_finder : public unary_function<space_t,bool> {
        std::wstring name ;
        space_finder(std::wstring nam) : name(nam) {}
        bool operator()(const space_t &spc) const {
          return spc.name==name ;
        }
      };
      std::vector<space_t> sources, arranges ;
      // 列挙
      for(DWORD vspc=0,tuner,spc;VirtualToRealSpace(vspc,tuner,spc);vspc++){
        sources.push_back(space_t(tuner,spc,Tuners[tuner].Spaces[spc].Name));
      }
      // 並替
      std::vector<space_t>::iterator beg=sources.begin(), end=sources.end();
      for(size_t i=0;i<SpaceArrangeNames.size();i++) {
        space_finder finder(SpaceArrangeNames[i]) ;
        remove_copy_if(beg, end, back_inserter(arranges), not1(finder)) ;
        end = remove_if(beg, end, finder) ;
      }
      copy(beg, end, back_inserter(arranges)) ;
      // 構築
      for(size_t i=0;i<arranges.size();i++)
        SpaceAnchors.push_back(arranges[i].anchor);
    }
  }
}
//-----
void CBonTuner::RotateTunerCandidates(size_t tuner)
{
  if(tuner<TunerPaths.size()&&TunerPaths[tuner].size()>=2)
    rotate(
      TunerPaths[tuner].begin(),
      TunerPaths[tuner].begin()+1,
      TunerPaths[tuner].end()) ;
  if(tuner<TunerModules.size()&&TunerModules[tuner].size()>=2)
    rotate(
      TunerModules[tuner].begin(),
      TunerModules[tuner].begin()+1,
      TunerModules[tuner].end()) ;
}
//-----
void CBonTuner::FreeTunerModules()
{
  for(size_t tuner=0;tuner<TunerModules.size();tuner++) {
    for(size_t i=0;i<TunerModules[tuner].size();i++)
      if(TunerModules[tuner][i]) FreeLibrary(TunerModules[tuner][i]) ;
  }
  TunerModules.clear() ;
}
//-----
void CBonTuner::PreloadTunerModules(bool nullAll)
{
  for(size_t tuner=TunerModules.size();tuner<TunerPaths.size();tuner++) {
    deque<HINSTANCE> modules ;
    for(size_t i=0;i<TunerPaths[tuner].size();i++) {
      HINSTANCE module = NULL ;
      if(!nullAll) {
        string tunerPath = TunerPaths[tuner][i] ;
        string relPath = MakeModulePath() + tunerPath ;
        module = LoadLibraryA(relPath.c_str()) ;
        if(!module) module = LoadLibraryA(tunerPath.c_str()) ;
      }
      modules.push_back(module) ;
    }
    TunerModules.push_back(modules);
  }
}
//-----
BOOL CBonTuner::ReloadTunerModule(size_t tuner, bool forceReload)
{
  if(!MagicSent)
    MagicSent=SendMagic() ;

  if(FullLoad) Tuners[tuner].Module=NULL ;
  Tuners[tuner].Free() ;

  string tunerPath = TunerPaths[tuner].front() ;
  wstring mutexName = L"BonReduction: "+mbcs2wcs(file_prefix_of(tunerPath)) ;

  if(ManageTunerMutex) {
    if(HANDLE Mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexName.c_str())) {
      // 既に使用中
      CloseHandle(Mutex) ;
      return FALSE ;
    }
    Tuners[tuner].Mutex = CreateMutex(NULL, TRUE, mutexName.c_str()) ;
  }

  if(!forceReload) {
    if(tuner<TunerModules.size()&&!TunerModules[tuner].empty()) {
      Tuners[tuner].Module = TunerModules[tuner][0] ;
    }
    forceReload = Tuners[tuner].Module ? false : true ;
  }

  if(forceReload) {
    string relPath = MakeModulePath() + tunerPath ;
    Tuners[tuner].Module = LoadLibraryA(relPath.c_str()) ;
    if(!Tuners[tuner].Module) {
      Tuners[tuner].Module = LoadLibraryA(tunerPath.c_str()) ;
    }
    if(tuner<TunerModules.size()&&!TunerModules[tuner].empty()) {
      if(TunerModules[tuner][0]) {
        FreeLibrary(TunerModules[tuner][0]) ;
      }
      TunerModules[tuner][0]=Tuners[tuner].Module;
    }
  }

  if(Tuners[tuner].Module) {
    CREATEBONDRIVER CreateBonDriver_
      = (CREATEBONDRIVER) GetProcAddress(Tuners[tuner].Module,"CreateBonDriver") ;
    if(CreateBonDriver_)
      Tuners[tuner].Tuner = static_cast<IBonDriver2*>(CreateBonDriver_()) ;
  }
  return Tuners[tuner].Tuner!=NULL ? TRUE : FALSE ;
}
//-----
BOOL CBonTuner::LoadTuner(size_t tuner,bool tuning,bool rotating)
{
  if(tuner>=TunerPaths.size()) return FALSE ;

  for(size_t i=Tuners.size();i<=tuner;i++) {
    Tuners.push_back(VTUNER(NULL,NULL)) ;
  }

  if(LazyOpen&&rotating&&Tuners[tuner].LastRotateFailed)
    return FALSE ;

  bool TunerClosed=!Tuners[tuner].Tuner ;
  BOOL Result = FALSE ;

  int n = (int) TunerPaths[tuner].size() ;
  bool bkRotating = rotating ;
  rotating = rotating && n>=2 ;

  if(TunerClosed) AsyncTSSuspend_() ;

  while(n>0) {

    if(!Tuners[tuner].Tuner) {
      BOOL reload_result = FALSE ;
      while(n--) {
        reload_result = ReloadTunerModule(tuner) ;
        if(reload_result) break ;
        if(!rotating) break ;
        if(n) RotateTunerCandidates(tuner);
      }
      if(!reload_result) break ;
    }

    if(OpenTunerOrdered) {
      if(CheckOpening) {
        if(Tuners[tuner].Opening) { // 何らかの理由で閉じられていないか厳密に調べる
          Tuners[tuner].Opening = Tuners[tuner].Tuner->IsTunerOpening() ;
          if(!Tuners[tuner].Opening) {
            if(CheckOpeningCloseOnFailure) {
              if(!TunerClosed) {
                AsyncTSSuspend_() ;
                TunerClosed=true ;
              }
              Tuners[tuner].Tuner->CloseTuner() ;
            }
            MagicSent=FALSE ;
          }
        }
      }
      if(!Tuners[tuner].Opening) {
        //チューナー実体のオープン処理
        for( DWORD t = Elapsed() ;
             !(Tuners[tuner].Opening=Tuners[tuner].Tuner->OpenTuner())&&
             Elapsed(t)<TunerRetryDuration ; ) {
          // 失敗した場合は、モジュールをロードし直してからやり直す
          if(ReloadOnTunerRetry&&!ReloadTunerModule(tuner,true)) break ;
        }
        if(!Tuners[tuner].Opening) {
          if(n&&rotating) {
            if(!TunerClosed) {
              AsyncTSSuspend_() ;
              TunerClosed=true ;
            }
            if(FullLoad) Tuners[tuner].Module=NULL ;
            Tuners[tuner].Free() ;
            continue ;
          }
          MagicSent=FALSE ;
          break ;
        }else {
          //チャンネルを設定する
          if(tuning) {
            if(CurRTuner==tuner) {
              BOOL channel_tuned = FALSE ;
              if(Tuners[tuner].Opening)
                channel_tuned
                 = Tuners[tuner].Tuner->SetChannel(CurRSpace,CurChannel) ;
              if(!channel_tuned) {
                if(n&&rotating) {
                  if(!TunerClosed) {
                    AsyncTSSuspend_() ;
                    TunerClosed=true ;
                  }
                  if(FullLoad) Tuners[tuner].Module=NULL ;
                  Tuners[tuner].Free() ;
                  continue ;
                }else
                  ResetChannel() ;
              }
            }
          }
        }
      }
    }
    Result = TRUE;
    break ;
  }

  if(TunerClosed) AsyncTSResume_() ;

  if(bkRotating)
    Tuners[tuner].LastRotateFailed = !Result ;
  return Result ;

}
//-----
LPCTSTR CBonTuner::GetTunerName(void)
{
  return TunerName.c_str() ;
}
//-----
const BOOL CBonTuner::IsTunerOpening(void)
{
  return OpenTunerOrdered ;
}
//-----
BOOL CBonTuner::VirtualToRealSpace(const DWORD dwSpace,DWORD &tuner,DWORD &spc)
{
  tuner = spc = 0 ;
  DWORD skipSpc = 0 ;
  //チューナー空間の列挙
  if(!SpaceAnchors.empty()) {
    // 並替＆列挙済
    if(dwSpace<SpaceAnchors.size()) {
      tuner = SpaceAnchors[dwSpace].Tuner ;
      spc = SpaceAnchors[dwSpace].Space ;
      return TRUE ;
    }
  }else do {
    while(tuner<Tuners.size()) {
      int n = static_cast<int>(Tuners[tuner].Spaces.size()) ;
      if(Tuners[tuner].MaxSpace==-1) {
        if(dwSpace+skipSpc<spc+n) {
          spc = dwSpace+skipSpc-spc ;
          return TRUE ;
        }else if(LoadTuner(tuner)) {
          while(spc+n<=dwSpace+skipSpc) {
            LPCTSTR name = Tuners[tuner].Tuner->EnumTuningSpace(n) ;
            if(!name) {
              Tuners[tuner].MaxSpace = n ;
              SaveIni() ; // 2012/11/21(Wed) 追加
              break ;
            }
            Tuners[tuner].Spaces.push_back(VSPACE(name)) ;
            if(spc+n==dwSpace+skipSpc) {
              spc = static_cast<DWORD>(Tuners[tuner].Spaces.size()-1) ;
              return TRUE ;
            }
            n++;
          }
        }else
          return FALSE ;
      }else {
        assert(n==Tuners[tuner].MaxSpace) ;
        for(int i=0;i<n;i++) {
          if(!Tuners[tuner].Spaces[i].Visible) {
            skipSpc++ ; continue ;
          }
          if(spc+i==dwSpace+skipSpc) {
            spc = i ;
            return TRUE ;
          }
        }
      }
      spc+=n ;
      tuner++ ;
    }
    if(tuner>=TunerPaths.size())
      break ;
  }while(LoadTuner(tuner)) ;
  return FALSE ;
}
//-----
BOOL CBonTuner::SerialToVirtualChannel(const DWORD SCh,DWORD &VSpace,DWORD &VCh)
{
  VSpace=VCh=0 ;
  DWORD ch = 0 ;
  while(ch<=SCh) {
    if(!EnumVirtualChannelName(VSpace,VCh)) {
      VSpace++,VCh=0 ;
      if(!EnumVirtualChannelName(VSpace,VCh))
        return FALSE ;
    }
    if(ch==SCh) {
      return TRUE ;
    }
    VCh++,ch++ ;
  }
  return FALSE ;
}
//-----
BOOL CBonTuner::VirtualToSerialChannel(const DWORD VSpace,const DWORD VCh,DWORD &SCh)
{
  DWORD vspc=0,vch=0;
  SCh=0;
  while(VSpace>=vspc) {
    if(!EnumVirtualChannelName(vspc,vch)) {
      vspc++,vch=0 ;
      if(!EnumVirtualChannelName(vspc,vch))
        return FALSE ;
    }
    if(vspc==VSpace) {
      if(vch==VCh) return TRUE ;
      else if(vch>VCh) return FALSE ;
    }
    vch++; SCh++;
  }
  return FALSE ;
}
//-----
LPCTSTR CBonTuner::EnumTuningSpace(const DWORD dwSpace)
{
  DoFullScan() ;
  if(SpaceConcat) {
    if(dwSpace==0) return SpaceConcatName.c_str() ;
  }else {
    exclusive_lock lock(&Exclusive) ;
    DWORD tuner,spc ;
    if(VirtualToRealSpace(dwSpace,tuner,spc))
      return Tuners[tuner].Spaces[spc].Name.c_str() ;
  }
  return NULL ;
}
//-----
LPCTSTR CBonTuner::EnumVirtualChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
  DWORD tuner,spc ;
  if(VirtualToRealSpace(dwSpace,tuner,spc)) {
    if(dwChannel<Tuners[tuner].Spaces[spc].Channels.size())
      return Tuners[tuner].Spaces[spc].Channels[dwChannel].Name.c_str() ;
    else if(Tuners[tuner].Spaces[spc].MaxChannel==-1) {
      if(!LoadTuner(tuner)) return NULL ;
      DWORD n = static_cast<DWORD>(Tuners[tuner].Spaces[spc].Channels.size()) ;
      while(n<=dwChannel) {
        LPCTSTR name = Tuners[tuner].Tuner->EnumChannelName(spc,n) ;
        if(!name) {
          Tuners[tuner].Spaces[spc].MaxChannel = n ;
          SaveIni() ; // 2012/11/21(Wed) 追加
          break ;
        }
        Tuners[tuner].Spaces[spc].Channels.push_back(static_cast<wstring>(name)) ;
        if(n==dwChannel) {
          return Tuners[tuner].Spaces[spc].Channels[n].Name.c_str() ;
        }
        n++;
      }
    }
  }
  return NULL ;
}
//-----
LPCTSTR CBonTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
  DoFullScan() ;
  exclusive_lock lock(&Exclusive) ;
  if(SpaceConcat) {
    if(dwSpace!=0) return NULL ;
    DWORD vspc,vch ;
    if(!SerialToVirtualChannel(dwChannel,vspc,vch)) return NULL ;
    return EnumVirtualChannelName(vspc,vch) ;
  }
  return EnumVirtualChannelName(dwSpace,dwChannel) ;
}
//-----
BOOL CBonTuner::GetCurrentTunedChannel(DWORD &curSpace,DWORD &curChannel)
{
  // 可能ならば現在のチャンネル情報をチューナー本体から抜き取る
  DoFullScan() ;
  if(CurRTuner>=Tuners.size()) return FALSE ;
  DWORD offsetSpc=0;
  DWORD hiddenNSpc=0 ;
  for(DWORD i=0;i<CurRTuner;i++) {
    if(Tuners[i].MaxSpace==-1) return FALSE ;
    for(int j=0;j<Tuners[i].MaxSpace;j++) {
      if(!Tuners[i].Spaces[j].Visible)
        hiddenNSpc++ ;
    }
    offsetSpc += Tuners[i].MaxSpace ;
  }
  if(Tuners[CurRTuner].MaxSpace==-1) return FALSE ;
  if(!Tuners[CurRTuner].Tuner) return FALSE ;
  CurRSpace = min<DWORD>( max<int>(0,Tuners[CurRTuner].MaxSpace-1),
    Tuners[CurRTuner].Tuner->GetCurSpace() ) ;
  if(SpaceAnchors.empty()) {
    for(size_t j=0;j<CurRSpace;j++) {
      if(!Tuners[CurRTuner].Spaces[j].Visible)
        hiddenNSpc++ ;
    }
    curSpace =  CurRSpace + offsetSpc - hiddenNSpc ;
    curChannel = Tuners[CurRTuner].Tuner->GetCurChannel() ;
    int MaxChannel = Tuners[CurRTuner].Spaces[CurRSpace].MaxChannel ;
    if(MaxChannel>=0) {
      curChannel = min<DWORD>( max<int>(0,MaxChannel-1), curChannel ) ;
    }
  }else {
    size_t i ;
    for(i=0;i<SpaceAnchors.size();i++) {
      VSPACE_ANCHOR &anchor = SpaceAnchors[i] ;
      if(anchor.Tuner==CurRTuner&&anchor.Space==CurRSpace) {
        curSpace = (int) i ;
        curChannel = Tuners[CurRTuner].Tuner->GetCurChannel() ;
        break;
      }
    }
    if(i>=SpaceAnchors.size()) return FALSE ;
  }
  return TRUE ;
}
//-----
BOOL CBonTuner::ResetChannel()
{
  if(GetCurrentTunedChannel(CurSpace,CurChannel)) {
    DBGOUT("RESET: CurSpace=%d, CurChannel=%d\r\n",CurSpace,CurChannel) ;
    return TRUE ;
  }
  return FALSE ;
}
//-----
BOOL CBonTuner::SetVirtualChannel(const DWORD dwSpace, const DWORD dwChannel)
{
  BOOL Result = FALSE ;
  DWORD old_tuner=CurRTuner,old_spc=CurRSpace ;
  if(FullOpen) VirtualToRealSpace(CurSpace,old_tuner,old_spc) ;
  DWORD tuner,spc ;
  if(VirtualToRealSpace(dwSpace,tuner,spc)) {
    size_t rotation_counter = TunerPaths[tuner].size() ;
    while(rotation_counter--) {
      if(LoadTuner(tuner,false,false)) {
        if(Tuners[tuner].Tuner->SetChannel(spc,dwChannel)) {
          CurSpace = dwSpace ; CurChannel = dwChannel ;
          CurRTuner = tuner ; CurRSpace = spc ;
          CurHasSignal = Tuners[CurRTuner].Spaces[CurRSpace].Channels[CurChannel].HasSignal ;
          CurHasStream = Tuners[CurRTuner].Spaces[CurRSpace].Channels[CurChannel].HasStream ;
          Result = TRUE ;
          break ;
        }
      }
      if(rotation_counter) {
        AsyncTSSuspend_() ;
        if(FullLoad) Tuners[tuner].Module = NULL ;
        Tuners[tuner].Free() ;
        RotateTunerCandidates(tuner) ;
        AsyncTSResume_() ;
      }
    }
    if(!Result) {
      if(!Tuners[tuner].Tuner)
        LoadTuner(tuner,false) ;
      if(Tuners[tuner].Tuner) {
        CurRTuner = tuner ;
      }
    }
    if(!FullOpen&&old_tuner!=CurRTuner) {
      // 開いている現行以外のチューナーをすべて閉じる
      AsyncTSSuspend_() ;
      for(size_t i=0;i<Tuners.size();i++) {
        if(i!=CurRTuner) {
          if(FullLoad) Tuners[i].Module = NULL ;
          Tuners[i].Free() ;
        }
      }
      AsyncTSResume_() ;
    }
  }
  if(Result) {
    // 予めパージを済ませておく
    if(CurRTuner!=old_tuner)
      Tuners[CurRTuner].Tuner->PurgeTsStream();
    if(AsyncTSEnabled) {
      exclusive_lock alock(&AsyncTSExclusive) ;
      if(AsyncTSFifo)
        AsyncTSFifo->Purge() ;
    }
  }else {
    // チューニングに失敗した場合は、現行チャンネルをチューナー本体から抜き取る
    if(LoadTuner(CurRTuner,false))
      ResetChannel() ;
  }
  DBGOUT("SetVirtualChannel: Space=%d, Channel=%d ",dwSpace, dwChannel) ;
  if(Result) DBGOUT("succeeded");
  else DBGOUT("failed");
  DBGOUT(".\r\n");
  return Result ;
}
//-----
const BOOL CBonTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
  exclusive_lock lock(&Exclusive) ;
  if(SpaceConcat) {
    if(dwSpace!=0) return FALSE ;
    DWORD vspc = 0 ,vch = 0 ;
    if(!SerialToVirtualChannel(dwChannel,vspc,vch)) return FALSE ;
    return SetVirtualChannel(vspc,vch) ;
  }
  return SetVirtualChannel(dwSpace,dwChannel) ;
}
//-----
const DWORD CBonTuner::GetCurSpace(void)
{
  DWORD result ;
  if(SpaceConcat) result= 0 ;
  else result = CurSpace ;
  DBGOUT("GetCurSpace: %d\r\n",result) ;
  return result ;
}
//-----
const DWORD CBonTuner::GetCurChannel(void)
{
  DWORD result ;
  if(SpaceConcat) {
    exclusive_lock lock(&Exclusive) ;
    DWORD SCh ;
    if(!VirtualToSerialChannel(CurSpace,CurChannel,SCh)) result=0 ;
    else result = SCh ;
  }else result = CurChannel ;
  DBGOUT("GetCurChannel: %d\r\n",result) ;
  return result ;
}
//-----
const BOOL CBonTuner::OpenTuner(void)
{
  exclusive_lock lock(&Exclusive) ;
  BOOL Result = TRUE ;
  if(FullLoad) PreloadTunerModules(!!LazyOpen) ;
  OpenTunerOrdered = TRUE ;
  do {
    if(LazyOpen) {
      if(CurRTuner>=TunerPaths.size())
        Result=FALSE ;
      else if(Tuners[CurRTuner].LastRotateFailed) //循環参照失敗済
        Result=FALSE ;
      break ; // Default
    }
    CurRTuner = min<DWORD>( CurRTuner , max(0,static_cast<int>(TunerPaths.size())-1) ) ;
    if(CurRTuner<TunerPaths.size()) {
      //exclusive_lock lock(&Exclusive) ;
      if(LoadTuner(CurRTuner,SaveCurrent?true:false)) {
        if(!SaveCurrent) ResetChannel() ;
        break ;
      }
    }
    Result =FALSE ;
  }while(0);
  if(!Result) {
    OpenTunerOrdered = FALSE ;
    return FALSE ;
  }
  DoFullScan();
  if(SignalAsBitrate) {
    SignalTickProceeding=GetTickCount() ;
    SignalBytesProceeded=0;
    SignalBitrate=0.f;
  }
  AsyncTSBegin() ;
  DBGOUT("Tuner Opened.\r\n");
  return TRUE ;
}
//-----
void CBonTuner::CloseTuner(void)
{
  exclusive_lock lock(&Exclusive) ;
  OpenTunerOrdered = FALSE ;
  MagicSent = FALSE ;
  SaveIni() ;
  AsyncTSEnd() ;
  {
    //exclusive_lock lock(&Exclusive) ;
    for(size_t tuner=0;tuner<Tuners.size();tuner++) {
      if(FullLoad) Tuners[tuner].Module=NULL ;
      Tuners[tuner].Free() ;
    }
  }
  DBGOUT("Tuner Closed.\r\n");
}
//-----
const BOOL CBonTuner::SetChannel(const BYTE bCh)
{
  if(!ByteTuning) return ByteTuningCancelResult ;
  exclusive_lock lock(&Exclusive) ;
  DWORD vspc = 0 ,vch = 0 ;
  if(!SerialToVirtualChannel(bCh,vspc,vch)) return FALSE ;
  return SetVirtualChannel(vspc,vch) ;
}
//-----
void CBonTuner::DoChannelKeeping()
{
  if(ChannelKeeping) {
    exclusive_lock lock(&Exclusive) ;
    if(LoadTuner(CurRTuner)) {
      DWORD curSpace,curChannel ;
      if(GetCurrentTunedChannel(curSpace,curChannel)) {
        if(curSpace!=CurSpace||curChannel!=CurChannel) {
          if(TunerPaths[CurRTuner].size()>=2) {
            AsyncTSSuspend_() ;
            if(FullLoad) Tuners[CurRTuner].Module = NULL ;
            Tuners[CurRTuner].Free() ;
            RotateTunerCandidates(CurRTuner) ;
            AsyncTSResume_() ;
          }
          if(!SetVirtualChannel(CurSpace,CurChannel)) {
            if(GetCurrentTunedChannel(curSpace,curChannel)) {
              //チューニング不可だった場合、現在のチャンネル情報に更新して諦める
              CurSpace = curSpace, CurChannel = curChannel ;
            }
          }
        }
      }
    }
  }
}
//-----
const float CBonTuner::GetSignalLevel(void)
{
  if(!OpenTunerOrdered||!CurHasSignal)
    return 0.0f ;
  DoChannelKeeping() ;
  exclusive_lock lock(&Exclusive) ;
  float result = 0.f;
  if(SignalAsBitrate) {
    //シグナルの代わりにビットレートMBpsを返す
    DWORD tick = GetTickCount() ;
    float duration ;
    duration = (float) Elapsed(SignalTickProceeding,tick) ;
    if (duration>1.f) {
        duration /= 1000.f;
        SignalBitrate = (float)SignalBytesProceeded*8.f / duration ;
        SignalTickProceeding = tick;
        SignalBytesProceeded = 0;
        result = SignalBitrate / 1024.f / 1024.f ;
        if (result > 100.f) result = 0.f;
    }
  }else {
    if(LoadTuner(CurRTuner))
      result = Tuners[CurRTuner].Tuner->GetSignalLevel() ;
  }
  return result ;
}
//-----
const DWORD CBonTuner::WaitTsStream(const DWORD dwTimeOut)
{
  if(!OpenTunerOrdered)
    return WAIT_FAILED ;
  /*else if(!CurHasStream)
    DoDropStream();
  */else{
    DoChannelKeeping() ;
    if(AsyncTSEnabled) {
      {
        exclusive_lock alock(&AsyncTSExclusive) ;
        if(AsyncTSFifo) {
          if(!AsyncTSFifo->Empty())
              return WAIT_OBJECT_0 ;
        }
      }
      return AsyncTSRecvEvent->wait(dwTimeOut) ; ;
    }else {
      exclusive_lock lock(&Exclusive) ;
      if(LoadTuner(CurRTuner))
        return Tuners[CurRTuner].Tuner->WaitTsStream(dwTimeOut) ;
    }
  }
  if(dwTimeOut&&dwTimeOut!=INFINITE)
    Sleep(dwTimeOut) ;
  return WAIT_TIMEOUT ;
}
//-----
const DWORD CBonTuner::GetReadyCount(void)
{
  if(!OpenTunerOrdered)
    return 0 ;
  /*else if(!CurHasStream)
    DoDropStream();
  */else {
    DoChannelKeeping() ;
    if(AsyncTSEnabled) {
      exclusive_lock alock(&AsyncTSExclusive) ;
      if(AsyncTSFifo) {
        return (DWORD)AsyncTSFifo->Size() ;
      }
    }else {
      exclusive_lock lock(&Exclusive) ;
      if(LoadTuner(CurRTuner))
        return Tuners[CurRTuner].Tuner->GetReadyCount() ;
    }
  }
  return 0 ;
}
//-----
const BOOL CBonTuner::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
  BOOL Result = FALSE ;
  if(!OpenTunerOrdered)
    return FALSE ;
  else if(!CurHasStream)
    DoDropStream();
  else {
    DoChannelKeeping() ;
    if(AsyncTSEnabled) {
      exclusive_lock alock(&AsyncTSExclusive) ;
      if(AsyncTSFifo) {
        BYTE *pSrc ;
        if(AsyncTSFifo->Pop(&pSrc,pdwSize,pdwRemain)) {
          alock.unlock() ;
          if(*pdwSize) {
            CopyMemory(pDst,pSrc,*pdwSize) ;
          }
          Result = TRUE ;
        }
      }
    }else {
      exclusive_lock lock(&Exclusive) ;
      if(LoadTuner(CurRTuner))
        Result = Tuners[CurRTuner].Tuner->GetTsStream(pDst,pdwSize,pdwRemain) ;
    }
    if(Result) {
      if(SignalAsBitrate) {
        if(pdwSize) SignalBytesProceeded+=*pdwSize ;
      }
    }
  }
  return Result ;
}
//-----
const BOOL CBonTuner::GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
  BOOL Result = FALSE ;
  if(!OpenTunerOrdered)
    return FALSE ;
  else if(!CurHasStream)
    DoDropStream();
  else {
    DoChannelKeeping() ;
    if(AsyncTSEnabled) {
      exclusive_lock alock(&AsyncTSExclusive) ;
      if(AsyncTSFifo) {
        AsyncTSFifo->Pop(ppDst,pdwSize,pdwRemain) ;
        Result=TRUE ;
      }
    }else {
      exclusive_lock lock(&Exclusive) ;
      if(LoadTuner(CurRTuner))
        Result = Tuners[CurRTuner].Tuner->GetTsStream(ppDst,pdwSize,pdwRemain) ;
    }
    if(Result) {
      if(SignalAsBitrate) {
        if(pdwSize) SignalBytesProceeded+=*pdwSize ;
      }
    }
  }
  if(!Result) {
    if(ppDst) *ppDst=NULL ;
    if(pdwSize) *pdwSize=0 ;
    if(pdwRemain) *pdwRemain=0 ;
  }
  return Result ;
}
//-----
void CBonTuner::DoDropStream()
{
  BYTE **ppDst=NULL; DWORD *pdwSize=NULL, *pdwRemain=NULL;
  if(AsyncTSEnabled) {
    exclusive_lock alock(&AsyncTSExclusive) ;
    if(AsyncTSFifo) {
      if(AsyncTSFifo->Size()>0)
        AsyncTSFifo->Pop(ppDst,pdwSize,pdwRemain) ;

    }
  }else {
    exclusive_lock lock(&Exclusive) ;
    if(LoadTuner(CurRTuner)) {
      if(Tuners[CurRTuner].Tuner->GetReadyCount()>0)
        Tuners[CurRTuner].Tuner->GetTsStream(ppDst,pdwSize,pdwRemain) ;
    }
  }
}
//-----
void CBonTuner::PurgeTsStream(void)
{
  exclusive_lock lock(&Exclusive) ;
  for(size_t i=0;i<Tuners.size();i++) {
    if(Tuners[i].Tuner) {
      if(Tuners[i].Opening)
        Tuners[i].Tuner->PurgeTsStream() ;
    }
  }
  if(AsyncTSEnabled) {
    exclusive_lock alock(&AsyncTSExclusive) ;
    if(AsyncTSFifo)
      AsyncTSFifo->Purge() ;
  }
}
//-----
void CBonTuner::AsyncTSBegin()
{
  if(AsyncTSEnabled) {
    AsyncTSEnd() ;
    AsyncTSRecvThread = (HANDLE)_beginthreadex(
      NULL, 0, AsyncTSRecvThreadProc, (PVOID)this, CREATE_SUSPENDED, NULL );
    if(AsyncTSRecvThread == INVALID_HANDLE_VALUE) {
        AsyncTSRecvThread = NULL;
    }
    if(AsyncTSRecvThread) {
      exclusive_lock alock(&AsyncTSExclusive) ;
      AsyncTSFifo = new CAsyncFifo(
        AsyncTSQueueNum,AsyncTSQueueMax,AsyncTSEmptyBorder,
        AsyncTSPacketSize,AsyncTSFifoThreadWait,AsyncTSFifoThreadPriority ) ;
      AsyncTSFifo->SetEmptyLimit(AsyncTSEmptyLimit) ;
      AsyncTSFifo->SetModerateAllocating(AsyncTSModerateAllocating?true:false) ;
      SetThreadPriority( AsyncTSRecvThread, AsyncTSRecvThreadPriority );
      AsyncTSRecvEvent = new event_object(FALSE) ;
      AsyncTSRecvDoneEvent = new event_object(FALSE) ;
      AsyncTSTerminated=FALSE ;
    }
    if(AsyncTSRecvThread)
      ResumeThread(AsyncTSRecvThread) ;
  }
}
//-----
void CBonTuner::AsyncTSEnd()
{
  if(AsyncTSEnabled) {
    AsyncTSTerminated=TRUE ;
    if(AsyncTSRecvThread) {
      HANDLE events[2] ;
      events[0]=AsyncTSRecvDoneEvent->handle() ;
      events[1]=AsyncTSRecvThread ;
      DWORD waitRes = WaitForMultipleObjects(2,events,FALSE,30000) ;
      if(waitRes==WAIT_OBJECT_0) {
        WaitForSingleObject(AsyncTSRecvThread,100) ;
        DBGOUT("AsyncTSRecvThread done.\r\n") ;
      }else if(waitRes==WAIT_OBJECT_0+1)
        DBGOUT("AsyncTSRecvThread terminated.\r\n") ;
      else {
        TerminateThread(AsyncTSRecvThread, 0);
        DBGOUT("AsyncTSRecvThread force terminated.\r\n") ;
      }
      CloseHandle(AsyncTSRecvThread);
      AsyncTSRecvThread = NULL;
    }
    {
      exclusive_lock alock(&AsyncTSExclusive) ;
      if(AsyncTSFifo) {
        delete AsyncTSFifo ;
        AsyncTSFifo = NULL ;
      }
    }
    if(AsyncTSRecvEvent) {
      AsyncTSRecvEvent->set() ;
      delete AsyncTSRecvEvent ;
      AsyncTSRecvEvent = NULL ;
    }
    if(AsyncTSRecvDoneEvent) {
      AsyncTSRecvDoneEvent->set() ;
      delete AsyncTSRecvDoneEvent ;
      AsyncTSRecvDoneEvent = NULL ;
    }
  }
}
//-----
unsigned int CBonTuner::AsyncTSRecvThreadProcMain()
{
  DWORD dwTimeOut = AsyncTSRecvThreadWait ;
  while(!AsyncTSTerminated) {
    DWORD rWait = WAIT_TIMEOUT ;
    IBonDriver2 *Tuner = NULL ;
    if(CurRTuner<TunerPaths.size()&&Tuners[CurRTuner].Opening) {
      Tuner = Tuners[CurRTuner].Tuner ;
    }
    if(Tuner) {
      if(Tuner->GetReadyCount()>0)
        rWait = WAIT_OBJECT_0 ;
      else
        rWait = Tuner->WaitTsStream(dwTimeOut) ;
    }else {
      if(dwTimeOut&&dwTimeOut!=INFINITE)
        Sleep(dwTimeOut) ;
    }
    if(rWait==WAIT_OBJECT_0) {
      DWORD n=0 ;
      BOOL r;
      BYTE *pData;
      DWORD dwSize,dwRemain ;
      do {
        pData=NULL; dwSize=dwRemain=0 ;
        r=Tuner->GetTsStream(&pData,&dwSize,&dwRemain) ;
        if(r&&pData&&dwSize) {
          AsyncTSFifo->Push(pData,dwSize) ;
          n++ ;
        }
        if(AsyncTSTerminated) {
          break ;
        }
      }while(r&&dwSize&&dwRemain) ;
      if(n) {
        AsyncTSRecvEvent->set() ;
      }
    }/*else if(rWait==WAIT_FAILED)
      break ;*/
  }
  return 0 ;
}
//-----
unsigned int __stdcall CBonTuner::AsyncTSRecvThreadProc(LPVOID pv)
{
  register CBonTuner *this_ = static_cast<CBonTuner*>(pv) ;
  unsigned int result = this_->AsyncTSRecvThreadProcMain() ;
  this_->AsyncTSRecvDoneEvent->set() ;
  _endthreadex(result) ;
  return result ;
}
//-----
BOOL CBonTuner::SendMagic()
{
  if(!MagicEnabled) return TRUE ;
  BOOL Result = FALSE ;
  CMagicClient mc ;
  vector<string> ipPortList ;
  vector<string> macList ;
  split(macList,MagicMACAddress,';') ;
  split(ipPortList,MagicIPEndPoint,';') ;
  for(size_t n=0;n<ipPortList.size();n++) {
    vector<string> ipPort ;
    split(ipPort,ipPortList[n],':') ;
    if(ipPort.empty()) continue ;
    string ip = ipPort[0] ;
    WORD port = MagicDefaultPort ;
    if(ipPort.size()>1) {
      port = atoi(ipPort[1].c_str()) ;
      if(!port) port = MagicDefaultPort ;
    }
    for(size_t m=0;m<macList.size();m++) {
      int iMac[6] ;
      string mac = macList[m] ;
      if(6==sscanf_s(mac.c_str(),"%02x:%02x:%02x:%02x:%02x:%02x"
          ,&iMac[0],&iMac[1],&iMac[2],&iMac[3],&iMac[4],&iMac[5])||
         6==sscanf_s(mac.c_str(),"%02x-%02x-%02x-%02x-%02x-%02x"
          ,&iMac[0],&iMac[1],&iMac[2],&iMac[3],&iMac[4],&iMac[5]) ) {
        BYTE btMac[6] ;
        for(int i=0;i<6;i++)
          btMac[i]=iMac[i]&255 ;
        DWORD retry = MagicRetry ;
        while(retry--) {
          mc.Open() ;
          if(mc.Send(ip,port,btMac)) {
            Result = TRUE ;
            mc.Close() ;
            break ;
          }
          mc.Close() ;
        }
      }else {
        DBGOUT("SendMagic: MAC \"%s\" の記述に間違いがある。\r\n",mac.c_str());
      }
    }
  }
  if(Result) {
    DBGOUT("SendMagic: マジックパケットの送信に成功。\r\n");
    if(MagicSleep>0)
      Sleep(MagicSleep) ;
  }else
    DBGOUT("SendMagic: マジックパケットの送信に失敗。\r\n");
  return Result ;
}
//-----
void CBonTuner::Release(void)
{
  if(RefCount==1)
    delete this ;
  else RefCount--;
}
//-----
void CBonTuner::AddRef()
{
  RefCount++ ;
}
//---------------------------------------------------------------------------
//===========================================================================
