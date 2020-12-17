(おまけ) Spinel Resume DAEMON

■説明

   Spinel.exe の死活監視とLogディレクトリのローテーションをほぼ自動で行うことを
  目的とした Spinel.exe の動作安定性を高めるためのおまけソフト。
   他にも複数の任意のディレクトリをローテーションする機能をも搭載。


■使い方

   Spinel.exe と同一ディレクトリに SpinelRD.exe と SpinelRD.ini を置いて、
  SpinelRD.exe を起動します。以降、バックグラウンドで死活監視活動に入ります。

  ※ 詳しい使い方は、添付の SpinelRD.ini ファイルの注釈を参照してください。
  ※ 本ソフトウェアは、バックグラウンドで動作します。終了する場合は、
    タスクマネージャから SpinelRD.exe を探してプロセスを終了してください。


■更新履歴

  ver. 1.1d

   ・ユーザーディレクトリローテーション機能にサブディレクトリを含めるかどうかを
    指定する項目をiniに追加(SubDirectories)

  ver. 1.1c

   ・サスペンド移行前にバックグラウンド進行中の処理をすべて停止する仕様に改良

  ver. 1.1b

   ・バックグラウンド処理を一時停止できる項目をiniに追加(JobPause)
   ・バックグラウンド処理の実行間隔を設定できる項目をiniに追加(JobInterval)
   ・バックグラウンド処理の優先度を設定できる項目をiniに追加(ThreadPriority)
   ・Spinelプロセスの優先度を設定できる項目をiniに追加(ProcessPriority)

  ver. 1.1

   ・SpinelRs から SpinelRD に名称変更(BDSからVC++に開発環境を移行)
   ・SpinelDeathResumeの機能を改善(Spinelが何らかの理由でフリーズした場合、その
    Spinelプロセスを一度完全に殺してからSpinelを再度立ち上げ直す仕組みに改良)
   ・タスクトレイにアイコンを常駐させる機能をiniに追加(ShowTaskIcon)


■免責事項

  無保証 ( NO WARRANTY )


以上ノシ
