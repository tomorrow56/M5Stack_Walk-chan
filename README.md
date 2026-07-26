# M5Stack_Walk-chan

A bipedal robot built with M5Stack.

![Walk-chan](img/Walk_chan.png)

## Overview

M5Stack Walk-chan（以下、Walk-chan）は、M5Stack を制御ボードとして使った2足歩行ロボットです。Otto DIY のような小型2足歩行ロボットを M5Stack サイズで実現することを目指し、2022年の「ｽﾀｯｸﾁｬﾝ（Stack-chan）」誕生1周年の誕生日会にあわせて設計されました。

このタイプの2足歩行ロボットで重要となるのは、片足を上げた際に接地している足側へ重心を確実に移すことです。M5Stack 本体だけでは重心移動が不十分なため、背面に搭載したバッテリーを左右に動かす直動機構を追加しています。これにより、安定した歩行動作を実現しています。

詳細な制作背景や動作動画については、note 記事「[二足歩行ｽﾀｯｸﾁｬﾝ(Walk chan)を作ってみた](https://note.com/tomorrow56/n/nf83f81b76e1e)」を参照してください。

## Features

- M5Stack 搭載の2足歩行ロボット
- 重心移動用のバッテリー直動機構（ラック＆ピニオン）
- サーボモータ SG90 を計5個使用
- M5Stack の M-Bus からサーボ PWM 用コネクタへ変換する基板同梱
- 3D プリント用 STL ファイルとアセンブリ済み STEP 形式のボディデータ
- Arduino IDE / PlatformIO 向けサンプルスケッチ
- LCD 上にアバター（目）を表示し、歩行と連動した演出が可能

## Hardware

### Required Parts

| 部品 | 数量 | 備考 |
|------|------|------|
| M5Stack Basic / Gray / Fire など | 1 | 制御ボード |
| SG90 互換サーボ | 5 | 2足（脚・足首）+ バッテリー移動用 |
| M5Stack Walk-chan 用基板 | 1 | 本リポジトリの `pcb/` 参照 |
| 3D プリント部品 | 一式 | `3Ddata/stl/` を参照 |
| リチウムイオン電池（NP-40 等） | 1 | コンパクトデジカメ用バッテリー推奨 |
| M3 ビス・ナット | 適量 | ギアガイドなどに使用 |

### Servo Assignment

サンプルスケッチでは、以下のように GPIO とサーボを割り当てています。

| サーボ | GPIO | LEDC チャンネル | 役割 |
|--------|------|-----------------|------|
| FootR  | 2    | 0               | 右足首（左右傾き） |
| FootL  | 12   | 1               | 左足首（左右傾き） |
| LegR   | 0    | 2               | 右脚（前後スイング） |
| LegL   | 15   | 3               | 左脚（前後スイング） |
| Ballast| 13   | 4               | バッテリー重心移動 |

サーボの中心位置や可動範囲は、個体差や組み立て状態に応じてスケッチ内の `cent*` 定数や `rot*` 定数を調整してください。

## 3D Model

ボディ部品の 3D データは `3Ddata/` に格納されています。

- `Walk-chan_assembled.step`：アセンブリ済みの STEP 形式ファイル（Fusion360 などで編集可能）
- `stl/`：3D プリント用の分割 STL ファイル
  - `bracket_front.stl`, `bracket_rear.stl`：前後ブラケット
  - `foot_l.stl`, `foot_r.stl`：左右の足
  - `joint_l.stl`, `joint_r.stl`：左右の脚関節
  - `gear.stl`, `rack_gear.stl`, `gear_guide.stl`：バッテリー直動機構用ギア
  - `holder_NP-40.stl`：NP-40 バッテリーホルダー
  - `shell.stl`：外装シェル

**注意：** 組み立て手順書は作成に至っていないため、現状ではアセンブリ済みの STEP ファイルを CAD で確認しながら組み立ててください。各部品の干渉やサーボの取り付け方向に注意してください。

## PCB for SG90

M5Stack の M-Bus からサーボ用 PWM コネクタに変換する基板データを `pcb/` に同梱しています。

- [gerber](pcb/gerver/)
- ![Schematic](pcb/Schematic.png)

### 基板の主な仕様

- M5Stack の M-Bus コネクタから直接各サーボの PWM 信号を出力
- サーボ制御 IC は使用せず、M5Stack の GPIO / LEDC 出力で直接駆動
- Grove 互換コネクタを引き出し、外部センサー等の拡張が可能
- サーボ電源は「コネクタから供給」または「M-Bus の 5V から供給」をハンダジャンパで選択可能
- 2pin ピンヘッダで「サーボ用 5V 電源」と「M5Stack 用リチウムイオン電池」を接続可能
- 横面の物理スイッチで M5Stack 本体の電源 ON/OFF が可能

## Sample Code

サンプルスケッチは `examples/M5_walk-chan_WalkTest/` に格納されています。

- `M5_walk-chan_WalkTest.ino`：メインスケッチ。歩行シーケンスとアバター表示、電池残量・充電状態の監視を行います。
- `clappyavator.h` / `clappyavator.cpp`：M5Stack の LCD に表示するアバター（目）を描画するライブラリ。
- `ip5306.h`：M5Stack 内蔵の電源管理 IC（IP5306）のレジスタ定義。

### 歩行シーケンスの概要

1. バラスト（バッテリー）を左右に移動し、重心を支持脚側へ移す。
2. 支持脚側の足首を内側・遊脚側の足首を外側に傾け、体を傾ける。
3. 遊脚の脚を前後に振り出す。
4. 足首を元に戻し、遊脚を地面に降ろす。
5. 反対側も同様に繰り返し、歩行を継続する。

`setup()` 内では上記の初期動作を1サイクル実施した後、`loop()` 内で左右交互に歩行動作を繰り返します。

### 充電モード

電源投入時に IP5306（0x75）が検出され、かつ充電中の場合は「HOME POSITION」調整モードに移行します。ボタン B を押すと通常の歩行デモに切り替わります。充電中にサーボが動作すると電源負荷や動作不安定の原因になるため、この安全機能が入っています。

## Build and Flash

1. 必要な部品を用意し、3D プリント部品を印刷します。
2. STEP ファイルまたは STL ファイルを参考に、サーボ・ギア・バッテリーを組み込みます。
3. 基板を製造・はんだ付けし、M5Stack と各サーボを接続します。
4. Arduino IDE または PlatformIO で `M5_walk-chan_WalkTest` を開き、M5Stack 向けにビルド・書き込みします。
   - 必要ライブラリ：`M5Stack` ライブラリ
5. 電源を入れ、M5Stack の LCD にアバターが表示され、Walk-chan が歩き始めることを確認します。

## Notes

- 基板のサーボコネクタはブラケットと干渉しない位置に配置されています。コネクタの向きに注意して配線してください。
- バッテリーケースはラックギヤにビスで固定されています。現在は NP-40 型のリチウムイオン電池用ホルダーが同梱されていますが、ケースを差し替えることで単4 乾電池など他の電源にも置き換え可能です。
- バッテリーの左右移動機構はブラケットにはめ込んだ交換可能な構造になっています。
- 2足歩行ロボットのため、床面やバランスによっては転倒することがあります。初動作時は周囲に十分なスペースをとり、低速で確認してください。

## References

- [note: 二足歩行ｽﾀｯｸﾁｬﾝ(Walk chan)を作ってみた](https://note.com/tomorrow56/n/nf83f81b76e1e)
- [Printables: M5Stack Walk-chan](https://www.printables.com/model/687241-m5stack-walk-chan)
- [tomorrow56 (ThousanDIY)](https://note.com/tomorrow56)

## License

[MIT License](LICENSE)
