- コンテナとは
  - 視野が制限されただけのただのプロセス。他プロセスとは独立した名前空間(PID、ネットワーク、マウントなど) を持つ
  - 技術的には、cloneシステムコールを用いて環境を分離して作られたプロセス

- 基礎
  - linux cgroups (Control Groups)
    - プロセスのリソースの使用量を制限、監視する
  - networking
    - コンテナ間及び、ホストとの通信を制御
  - /proc
    - プロセスが管理されているディレクトリ
    - ディレクトリはあるが実態はkernelのプロセスのメモリ上に存在する
    - ファイルシステムのIFを使ってファイルを操作するかのようにメモリ上のプロセス情報を参照できる

- nsexec
  - 流れ
    - コマンド実行時のオプションの解析
    - child_args構造体に設定しつつ、cloneシステムコールを呼び出し、子プロセスを複製する
      - このとき、オプションによって渡すflagを変える
      - 例えば-pがあった場合はCLONE_NEWPIDのフラグを追加し、pid namespaceの分離をする
    - 親から子プロセスに実行開始の通知をする
      - 通知はpipe_fdを用いる
    - 子プロセス側の処理 (主にchildFunc)
      - 子プロセス側では親からの通知が来てからマウント処理を開始する
        - 通知を待つ理由は、技術的に大きな問題があるわけではないが、親/子で実行順序を保証するための仕組みだと考えられる
        - マウント関連の処理は、以下の2ステップがある
          - プライベート化
            - トップ("/")ディレクトリの配下にあるものすべてを再起的にプライベートにしていく。これによりこのプロセス内での操作を他のプロセスに影響を与えないように分離
              - このとき/procなど親の持っていたデータはそのまま複製されている
          - procのアンマウント/新規マウントによるプロセスの分離
            - プライベート化しただけでは、他に影響を与えないとはいえ前述したように親の/procの情報などが残っている
            - /procをアンマウントし、新たに/procをマウントすることでclone時に作成されたPID名前空間用のproc情報に切り替える
            - これにより新しい名前空間のプロセスのみが見える
              - 逆に切り替える前までは、子プロセスの所属するPID名前空間以外も見えており(親のPID名前空間)、不整合な状態
            - これらは、マウント名前空間での参照の調整により、PID名前空間用のプロセス情報表示に切り替える処理
      - 必要に応じて、sethostnameシステムコールを用いてhostnameの設定をする
      - 最後にexecvpシステムコールを用いて、./nsexec実行時に受け取った引数があれば渡して実行する
        - 例えばbashを受け取った場合は、bashプロセスがstackなどメモリ空間をすべて初期化し、元の情報が消えてbashとして生まれかわる
        - このときpipe_fdなどのデータも消えるので親とのやりとりはできなくなるが、親側ではPIDだけ確認できているので、一方的に監視は可能
        - また、cloneはヒープなどのデータはコピーオンライトになっているため、書き込み時にコピーされるが、bashの初期化によりそれらの参照もなくなる
    - 親側では子プロセスのPIDをwaitpidシステムコールを用いて監視し、terminateされたのを確認したら親(ここでは./nsexecの実行プロセス) も終了する
    - ちなみに親側からも子プロセスの/procを参照することはできる
      - 場所としては/proc/子のPID/root/proc/* にある
