# diyC

シンプルなLinuxコンテナランタイム(C言語で500行程度) をゼロから作るプロジェクトです。

ソースは[こちら](https://github.com/w-vi/diyC)をベースにしています。

## 概要

- **diyC**: シンプルなLinuxコンテナランタイム実装
- **nsexec**: 指定された名前空間でコマンドを実行する簡易ツール

## Getting Started

### nsexec

docker composeの起動とbashで接続
```bash
docker-compose up -d && docker-compose exec diyc bash
```

コンテナ内でセットアップ
```
make setup && make
```

nsexecの実行
```
./nsexec -pnu hoge bash
```

動作確認
```
# PID名前空間の分離
root@hoge:/workspace# ps aux
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root         1  0.0  0.0   4116  3200 pts/2    S    09:30   0:00 bash
root         7  0.0  0.0   6408  2432 pts/2    R+   09:40   0:00 ps aux

# ネットワークの分離
root@hoge:/workspace# ip addr show
1: lo: <LOOPBACK> mtu 65536 qdisc noop state DOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: tunl0@NONE: <NOARP> mtu 1480 qdisc noop state DOWN group default qlen 1000
    link/ipip 0.0.0.0 brd 0.0.0.0
3: ip6tnl0@NONE: <NOARP> mtu 1452 qdisc noop state DOWN group default qlen 1000
    link/tunnel6 :: brd :: permaddr ce71:fef5:6c83::

# UTS名前空間の分離
root@hoge:/workspace# hostname 
hoge
```

## 備考
- **学習ガイド(準備中)**: [docs/README.md](docs/README.md)
- **個人メモ**: [docs/MEMO.md](docs/MEMO.md)
- **ソースコード**: `src/nsexec.c`, `src/diyc.c`
