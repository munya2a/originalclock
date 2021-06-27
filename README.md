# OriginalClock

見やすく、簡単に秒の表示 /非表示が切り替えられ、
柔軟にカスタマイズできる、リナックスのデスクトップの
アナログ時計です

This is a simple linux desktop analog clock application.
Easy to see, easy to switch display/hide seconds
and fully customizable.

## コンパイルと設定ファイルのコピー

解凍して、マウスメニューから「端末の中に開く」で
コマンドを開いてコンパイル
```
g++ `pkg-config --cflags gtk+-3.0` -o originalclock main.cpp `pkg-config --libs gtk+-3.0`
```

ユーザーのホームディレクトリにテーマをコピー
 (ディレクトリの前の. は隠しを意味します)
```
cp -r orgclock $HOME/.orgclock
```
新しいバージョンを実行する場合、一旦、以前の設定ファイル $HOME/.orgclock/param.dat を削除してください

## 実行
```
./originalclock
```

## もしくはmake, make installします

作成者のシステム上で
```
aclocal
autoconf
automake --add-missing
./configure
make distcheck
```

利用者のシステム上で
```
./configure
make
make install
```

ユーザーのホームディレクトリにテーマをコピー
```
cp -r orgclock $HOME/.orgclock
```
