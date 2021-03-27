BonDriver_RDCT ( BonDriver_Reduction )

■説明

  EDCB に WOL機能付の BonDriver_Spinel を読み込ませると片っ端からマジックパケ
  ットを送りまくる現象の回避のために EDCB と BonDriver_Spinel の中間に組み込む
  ためのラッパー。

  Spinel を数珠繋ぎにすることにより、マジックパケットの送信を回避することは可
  能ではあるものの、マジックパケットで起動したコンピュータがスタンバイ状態に
  入ると Spinel 側がソケットが切断された状態を認知できずにマジックパケットを
  送るタイミングを見誤るので、代用になるラッパーのようなものを作ってみた。

  このラッパーを利用することにより、BonDriverの機能をカプセル化して省略可能な
  事前処理を引き受けることによってBonDriverインスタンスの生成を極限まで抑える
  ことができる筈…

  Spinel環境でのテストでしか動作確認していないため過度な期待は禁物。


■使用方法

  EDCB での動作環境での説明

    ●ディレクトリ構成

    EpgDataCap_Bon.exe
    └<BonDriver>
       ├BonDriver_RDCT.dll
       ├BonDriver_RDCT.ini
       └<RDCT>
         ├BonDriver_Spinel.dll.ini
         └BonDriver_Spinel.dll

    ●BonDriver_RDCT.ini の内容

     ----------------------------
     [Setting]
     ;TunerName (必須)
     ; チューナーの名前
     TunerName=RDCT:Spinel_PT-T0 とか

     ;TunerPath (必須)
     ; BonDriver 本体のパス (相対パス、またはフルパス)
     TunerPath=RDCT\BonDriver_Spinel.dll
     ----------------------------

  一度EDCBプログラムを起動すると、.transit.txt ファイルが自動で生成され、
  チャンネル情報が記述される（編集してチャンネルの名前を変える事も可能）。

  二度目の起動からは、.transit.txt ファイルに出力された情報からチューナーを模倣
  するのでBonDriver本体のインスタンスの生成を極限まで抑えることができるように
  なる。注意点としてBonDriver本体のチャンネル情報を変更した場合は .transit.txt
  ファイル自体を抹消しないとチャンネル情報がおかしくなるので気をつけること。

  BonDriver_RDCT.dll を名前を変更することによって BonDriver_Spinel のように複製
  が可能。名前を変えた場合は .ini ファイルの名前も同じものにすること。

  おまけのような機能として複数のチューナーを一つのチューナーとして利用する
  ことも可能。地デジチューナーとBS/CSチューナーをミックスして３波として扱ったり
  することができる。

  TVTest で三波化の例

    ●ディレクトリ構成

    TVTest.exe
    └<BonDriver>
       ├BonDriver_RDCT_PT-TS.dll  ← BonDriver_RDCT.dll をリネームしたもの
       ├BonDriver_RDCT_PT-TS.ini  ← BonDriver_RDCT.ini をリネームしたもの
       └<RDCT>
         ├BonDriver_Spinel_PT-T0.dll.ini
         ├BonDriver_Spinel_PT-T0.dll     ← <例> PT2 の T0 ストリーム
         ├BonDriver_Spinel_PT-S0.dll.ini
         └BonDriver_Spinel_PT-S0.dll     ← <例> PT2 の S0 ストリーム


    ●BonDriver_RDCT.ini の内容

     ----------------------------
     [Setting]
     ;TunerName (必須)
     ; チューナーの名前
     TunerName=RDCT:Spinel_PT-TS

     ;TunerPath[0..n] 何個でも追加可能 (必須)
     ; BonDriver 本体のパス (相対パス、またはフルパス)
     TunerPath0=RDCT\BonDriver_Spinel_PT-T0.dll
     TunerPath1=RDCT\BonDriver_Spinel_PT-S0.dll

     ;FullOpen
     ; 一度開いたチューナーを開きっぱなしにするかどうか
     ; 1 にするとチューナー間のチャンネル切り替えが高速になるが、
     ; 開いているチューナーの分だけリソースが浪費される
     ; (Default:0)
     FullOpen=1

     ;RecordTransit
     ; .transit.txt ファイルに遷移を記録するかどうか
     ; (Default:1)
     RecordTransit=0
     ----------------------------

     ※ Version 1.1 より TunerPath に ";" で区切ることにより複数のスペアとなる
       BonDriver が追記可能となっている。

       スペアになる BonDriver は、最初に指定した BonDriver が開けないかチューニ
       ングできないときに自動的に循環参照される。

       TunerPath=RDCT/BonDriver_1.dll;RDCT/BonDriver_2.dll;RDCT/BonDriver_3.dll

       たとえば、上記の場合は、BonDriver_1 が開けないときは、BonDriver_2 を
       BonDriver_2 が開けないときは、BonDriver_3 を開くように順々に試行する。

       ; で区切られた BonDriver の互換性は、スペースとチャンネルが全く同じ配置
       いわば、完全なるクローンでなければならないことに注意すること。


■FAQ

  Q: Spinel が排他で使用中、排他でない BonDriver_Spinel を BonDriver_RDCT に
     ラップして割込ませて使用するとTVTestに映像が表示されないのは仕様なのか？
     排他のチューナーと同じチャンネルに自動でチューニングできないのか？

  A: これはTVTest固有の仕様なのか、ファイル名を変えることで回避できるようで、
     たとえば BonDriver_RDCT_PT-TS.dll というラップしたファイル名を利用して
     いる場合 BonDriver_RDCT_Spinel_PT-TS.dll にして(何処かしら Spinel という
     名前をファイル名に含めて)みると解決するかもしれない。


■更新履歴

 version 1.5 rev.5

  ・割込みタイマーの精度を高める項目 MMTimerEnabled をiniに追加[rev.5]
  ・チューナーオープン再試行時にチューナー候補を回し忘れていたバグを修正[rev.4]
  ・チャンネル切替時にチューナー候補を余計に回してしまうバグを修正[rev.3]
  ・チューナー候補の切替が上手くいかなくなることがある現象を修正[rev.2]
  ・チューナーオープンに費やす最大試行時間の項目 TunerRetryDuration をiniに追加
  ・チューナー空間の並べ替えが出来る項目 SpaceArrangement をiniに追加
  ・チューナーモジュールの一気読みを設定する項目 FullLoad をiniに追加

 version 1.4 rev.4

  ・非同期TSストリームスレッドの優先順位を設定できる項目をiniに追加[rev.4]
  ・dB欄に転送量Mbpsを代替表示する SignalAsBitrate フラグをiniに追加[rev.3]
  ・Transitファイルの TunerX.SpaceX.ChannelX に HasSignal フラグを追加[rev.3]
    ( TunerX.SpaceX.ChannelX.HasSignal=0 にすると所定のシグナルを無効化 )
  ・Transitファイルの TunerX.SpaceX.ChannelX に HasStream フラグを追加[rev.3]
    ( TunerX.SpaceX.ChannelX.HasStream=0 にすると所定のストリームを停止 )
  ・非同期バッファリングが有効な場合にアクセス違反が発生するバグを修正[rev.3]
  ・LazyOpenが有効なときにTunerPathで複数指定したすべてのチューナーが開けなかっ
    た場合にチューナーパスを無限に循環参照することのあるバグを修正[rev.3]
  ・マジックパケット送信を有効化するフラグ MagicEnabled をiniに追加[rev.2]
  ・他のクライアントがチャンネルを切り替えたときに自動でチャンネルをチューニ
    ングし直す ChannelKeeping フラグをiniに追加[rev.1]
  ・複数のスペアとなるBonDriverを使いまわしているときに LazyOpen が有効になっ
    ていない場合、スペアのBonDriverを全く試行しないことがあるバグを修正[rev.1]
  ・TSストリームの非同期バッフリング機能を追加

 version 1.3 rev.2

  ・複数のチューナーを管理するための ManageTunerMutex フラグをiniに追加[rev.2]
  ・複数のチューナー空間を一つに連結するための SpaceConcat フラグをiniに追加

 version 1.2 rev.2

  ・CBonTuner::VirtualToRealSpace メソッドバグ修正[rev.2]
  ・CTransitProfiler::Flush メソッドバグ修正[rev.1]
  ・.ini ファイルの Transit セクションに書き出していた遷移情報を .transit.txt
    ファイルに書き出す独自ルーチンに変更

 version 1.1 rev.2

  ・チューナーを閉じた時点で遷移情報を Transit セクションに一遍に書き出して
    いた処理をチューナースペース毎のチャンネルの列挙が終わった時点でこまめに
    書き出すようにiniファイルに保存する新たな処理のタイミングを追加[rev.2]
  ・チャンネルスキャンが不完全な状態で保存されたiniファイルを次回起動時に
    読み込むとプログラムがハングアップを起こすことがあるバグを修正[rev.1]
  ・TunerPath にスペアとなる BonDriver を追記できるように改良
  ・iniファイルに保存するタイミングを修正

 version 1.0

  ・Transit セクション TunerX.SpaceX ノードに Visible フラグを追加
    ( TunerX.SpaceX.Visible=0 にすると所定のスペースを無効化 )
  ・Transit セクションの無駄書き処理を防止
  ・チャンネルの設定に失敗した場合、チューナー本体のチャンネル情報を抜き取って
    もってくる仕様に変更
  ・バージョン付初回リリース


■免責事項

  無保証 ( NO WARRANTY )


以上ノシ
