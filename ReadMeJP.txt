                      ------------------------------
                                DoWonders
                           （ドゥーヲンダーズ）
                                     
                        Win32 API Database Project
                              by 片山博文MZ
                      ------------------------------

+----+
|概要|
+----+
これは、Win32 API データベースプロジェクト (DoWonders) です。

100 ％公開技術です。

DoWonders は、C/Win32 パーサー (cparser)、ハックされた C プリプロセッサー 
(mcpp-hacked)、そして DLL 情報のダンパー (dllexpdumper) を含みます。

cparser は、Win32 API に関する型情報を抽出します。mcpp-hacked は、マクロ情
報を抽出します。dllexpdumper は、DLL 情報を抽出します。

+------+
|使い方|
+------+
まずは、フォルダー「tools」の中のツールを Visual C++ により、ひとつづつ構築
して下さい。ツールの構築は、次の四通りありますので注意して下さい。

    * Win32 + Debug
    * Win32 + Release
    * x64 + Debug
    * x64 + Release

ツールの構築が終わったら、64 ビットの場合は「do-wonders-cl-64.bat」を 64 ビ
ットの Visual Studio (VS) のコマンド プロンプトから、 32 ビットの場合は
「do-wonders-cl-32.bat」を 32 ビットの VS のコマンド プロンプトから実行して
下さい。データベースの構築が始まります。

すべて終わったら、拡張子が「.dat」のファイルがあちこちに作成されます。でき
たファイルは、ご自由にお使い下さい。

"sanitize-cl-32.bat" か "sanitize-cl-64.bat" でデータベースをチェックできま
す。

+----+
|状態|
+----+
Visual C++ 2013 と TDM-GCC-64 + MSYS でテストしています。

現在、typed valueを実装し、サニタイズ中です。

注記: GCC におけるデータベースは、サニタイズできなかった。なぜなら、GCC は
      いくつかのバグと Visual C++ との差異を含んでいるからだ。我々は、Visual
      C++ の振る舞いを信頼している。

+--------+
|やること|
+--------+
 * C パーサーを改良する。
 * Win32 API データベースのWebサイトを作成する。
 * 主要言語のためにWin32 APIへのアクセスを提供する。
 * 新しい Win32 API の世界を作成する。


/////////////////////////////////////////////////////
// 片山博文MZ (katahiromz) [蟻]
// ホームページ http://katahiromz.web.fc2.com/
// 掲示板       http://katahiromz.bbs.fc2.com/
// メール       katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////
