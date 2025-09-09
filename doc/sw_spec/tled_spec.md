# TLEDタスク ソフトウェア仕様書

## はじめに
本仕様書は、Renesas EK-RA8D1ボード上に実装された3色LED（赤・青・緑）を制御する **TLEDタスク** の機能と動作仕様を定義するものである。  
本タスクはT-Kernel上で動作し、他タスクからの要求に基づきLEDの点灯・点滅制御を行い、動作完了の通知を返す。

---

## 機能概要
- RA8D1ボード上の3つのLED（赤・青・緑）を独立して制御する。
- LED制御要求を **TAPP等の他タスクからメールボックス経由で受信**し、内部状態機械で制御を実行する。
- LED点灯状態は100ms周期で更新され、指定されたパターンに従って駆動される。
- 要求の受理後は応答メッセージを返すことで、呼び出し元との同期を図る。

---

## 機能詳細

### 1. 対象LED
- USER_LED3_RED  
- USER_LED3_BLUE  
- USER_LED3_GREEN  
（FSP Configuratorにて `ioport` シンボルとして定義済み）

### 2. メッセージ仕様
- **要求メッセージ (MSGID_TLED_REQ)**  
  ペイロード `tled_req_t`:
  - `led`: LED識別子 (`TLED_RED` / `TLED_BLUE` / `TLED_GREEN`)
  - `pattern`: 制御パターン  
    - `TLED_PAT_OFF`: 消灯  
    - `TLED_PAT_ON`: 点灯  
    - `TLED_PAT_BLINK_FAST`: 速点滅 (100ms ON / 100ms OFF)  
    - `TLED_PAT_BLINK_SLOW`: 遅点滅 (500ms ON / 500ms OFF)
  - `blink_count`: 点滅回数（有限回数または `TLED_BLINK_INFINITE` 指定）

- **応答メッセージ (MSGID_TLED_RES)**  
  - ペイロードなし  
  - 要求元タスクに送信され、受理を通知する。

### 3. 点滅制御
- タスクは100ms周期で内部状態を更新する。
- 各LEDは独立した状態機械で管理され、点灯/消灯のトグルを行う。
- 点滅回数が指定されている場合、ON状態へ遷移するたびに1回分を消費する。
- 残り回数が0になると、自動的に消灯して終了する。
- `TLED_BLINK_INFINITE` が指定された場合、無限に点滅を継続する。

### 4. 上書き仕様
- 制御中に新たな要求を受信した場合、現在の制御は中断され、新しい要求が即座に適用される。

### 5. ハードウェア制御
- LEDはGPIO経由で制御される。
- 出力レベルは **アクティブLow** を想定し、  
  - `LED_ON_LEVEL = BSP_IO_LEVEL_LOW`  
  - `LED_OFF_LEVEL = BSP_IO_LEVEL_HIGH`  
  と定義している。必要に応じて変更可能。

---

## 状態遷移図

```mermaid
stateDiagram-v2
    [*] --> OFF

    OFF --> ON       : MSGID_TLED_REQ (点灯)
    OFF --> BLINK    : MSGID_TLED_REQ (点滅)

    ON --> OFF       : MSGID_TLED_REQ (消灯)
    ON --> BLINK     : MSGID_TLED_REQ (点滅)

    BLINK --> OFF    : 点滅回数終了
    BLINK --> BLINK  : ON↔OFFトグル (100ms/500ms周期)
    BLINK --> OFF    : MSGID_TLED_REQ (消灯)
    BLINK --> ON     : MSGID_TLED_REQ (点灯)

    OFF --> OFF      : MSGID_TLED_REQ (消灯)
    ON --> ON        : MSGID_TLED_REQ (点灯)
