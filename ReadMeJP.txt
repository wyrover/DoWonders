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

+----+
|状態|
+----+
Visual C++ 2013 Express でテストしました。

    --------------------
               お め で と う ご ざ い ま す !!!
                                    ------------------

Win32 APIのほとんど全ての情報がベールをぬぎました！
型、構造体、関数、マクロ、そしてすべて。

iwonit プログラム (Won32 interactive) 付きのすべてのデータが Wonders API
公式サイトからダウンロード可能です。

    Wonders API 公式サイト
    http://katahiromz.esy.es/wonders/

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
ットの Visual Studio (VS) のコマンド プロンプト (x64) から、 32 ビットの場
合は「do-wonders-cl-32.bat」を 32 ビットの VS のコマンド プロンプト (x86)
から実行して下さい。データベースの構築が始まります。

すべて終わったら、拡張子が「.dat」のファイルがあちこちに作成されます。でき
たファイルは、ご自由にお使い下さい。

"sanitize-cl-32.bat" か "sanitize-cl-64.bat" でデータベースをチェックできま
す。

+--------+
|やること|
+--------+
 * Win32 API データベースのWebサイトを作成する。
 * 主要言語のためにWin32 APIへのアクセスを提供する。
 * 新しい Win32 API の世界を作成する。

+------------+
|商標について|
+------------+

Microsoft、Windows、およびWin32 API は、マイクロソフト社の登録商標です。

/////////////////////////////////////////////////////
// 片山博文MZ (katahiromz) [軍隊蟻]
// ホームページ http://katahiromz.web.fc2.com/
// 掲示板       http://katahiromz.bbs.fc2.com/
// メール       katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////
