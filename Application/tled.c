/*
 * tled.cpp - LED制御タスク
 *
 * 仕様:
 *  - 他タスクからの MSGID_TLED_REQ を受信し、LED(赤/青/緑)を独立制御
 *  - パターン: 点灯, 消灯, 速点滅(100ms), 遅点滅(500ms)
 *  - 点滅回数: n回 または 無限(TLED_BLINK_INFINITE)
 *  - 100ms周期で内部タイマ駆動
 *  - 応答: MSGID_TLED_RES（同期用。ペイロードなし）
 */

#include "tled.h"

/* ---- RA8D1 EK のユーザLEDは「アクティブHigh ---- */
#define LED_ON_LEVEL    BSP_IO_LEVEL_HIGH
#define LED_OFF_LEVEL   BSP_IO_LEVEL_LOW

/* ピン割当（FSP Configurator で定義済みシンボルを使用） */
static const ioport_port_pin_t kLedPin[TLED_NUM] = {
    USER_LED1_BLUE,
    USER_LED2_GREEN,
    USER_LED3_RED
};

/* 100msタイマTick */
#define TLED_TICK_MS          (100)

/* ブリンク周期（Tick単位） */
#define TLED_FAST_ON_TICKS    (1)  /* 100ms ON */
#define TLED_FAST_OFF_TICKS   (1)  /* 100ms OFF */
#define TLED_SLOW_ON_TICKS    (5)  /* 500ms ON */
#define TLED_SLOW_OFF_TICKS   (5)  /* 500ms OFF */

/* LED状態機械 */
typedef struct {
    tled_pattern_t  pat;
    W               blink_rem;   /* 残り点滅回数 (-1: 無限) */
    bool            is_on;       /* 現在の出力状態 */
    UW              phase_ticks; /* 現フェーズの残りTick数 */
} tled_sm_t;

static tled_sm_t s_led[TLED_NUM];

/* ---- ユーティリティ ---- */

/* 物理出力 */
static inline void led_write(tled_id_t id, bool on)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, kLedPin[id], on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

/* 状態→出力セット & 初期フェーズ長を設定 */
static void led_apply_pattern(tled_id_t id, tled_pattern_t pat, W count)
{


    tled_sm_t *pls = &s_led[id];

    pls->pat = pat;
    pls->blink_rem = count;

    switch (pat) {
    case TLED_PAT_OFF:
        pls->is_on = false;
        pls->phase_ticks = 0;
        led_write(id, false);
        break;

    case TLED_PAT_ON:
        pls->is_on = true;
        pls->phase_ticks = 0;
        led_write(id, true);
        break;

    case TLED_PAT_BLINK_FAST:
        /* ONから開始: 1サイクル= ON(1)→OFF(1) */
        pls->is_on = true;
        pls->phase_ticks = TLED_FAST_ON_TICKS;
        led_write(id, true);
        break;

    case TLED_PAT_BLINK_SLOW:
        /* ONから開始: 1サイクル= ON(5)→OFF(5) */
        pls->is_on = true;
        pls->phase_ticks = TLED_SLOW_ON_TICKS;
        led_write(id, true);
        break;
    }
}

/* ブリンクの1サイクル消化をカウントダウン（必要なら） */
static inline void led_consume_cycle_if_needed(tled_id_t id, bool toggled_to_on)
{
    tled_sm_t *pls = &s_led[id];
    if (pls->pat == TLED_PAT_BLINK_FAST || pls->pat == TLED_PAT_BLINK_SLOW)
    {
        /* 「ONに入った瞬間」を1回としてカウント */
        if (toggled_to_on && pls->blink_rem > 0) {
            pls->blink_rem--;
        }
    }
}

/* 100msごとのタイマ処理（各LEDを更新） */
static void led_tick(void)
{
    for (int i = 0; i < TLED_NUM; i++) {
        tled_sm_t *pls = &s_led[i];

        switch (pls->pat) {
        case TLED_PAT_OFF:
        case TLED_PAT_ON:
            /* 固定状態。何もしない */
            break;

        case TLED_PAT_BLINK_FAST:
        case TLED_PAT_BLINK_SLOW:
        {
            if (pls->phase_ticks > 0) {
                pls->phase_ticks--;
                if (pls->phase_ticks > 0) break;
            }

            /* フェーズ終了 → トグルして次フェーズへ */
            if (pls->pat == TLED_PAT_BLINK_FAST) {
                if (pls->is_on) {
                    /* ON→OFFへ */
                    pls->is_on = false;
                    pls->phase_ticks = TLED_FAST_OFF_TICKS;
                    led_write((tled_id_t)i, false);
                } else {
                    /* OFF→ONへ */
                    pls->is_on = true;
                    pls->phase_ticks = TLED_FAST_ON_TICKS;
                    led_write((tled_id_t)i, true);
                    /* ONに入ったので点滅回数を消費(有限のとき) */
                    if (pls->blink_rem > 0) pls->blink_rem--;
                }
            } else { /* SLOW */
                if (pls->is_on) {
                    pls->is_on = false;
                    pls->phase_ticks = TLED_SLOW_OFF_TICKS;
                    led_write((tled_id_t)i, false);
                } else {
                    pls->is_on = true;
                    pls->phase_ticks = TLED_SLOW_ON_TICKS;
                    led_write((tled_id_t)i, true);
                    if (pls->blink_rem > 0) pls->blink_rem--;
                }
            }

            /* 有限回数が尽きたら消灯で終了 */
            if (pls->blink_rem == 0) {
                pls->pat = TLED_PAT_OFF;
                pls->is_on = false;
                pls->phase_ticks = 0;
                led_write((tled_id_t)i, false);
            }
        } break;
        } /* switch pat */
    } /* for */
}

/* 応答送信: MSGID_TLED_RES（ペイロードなし） */
static ER send_led_res(INT result, ID dsttsk)
{
    ER er;
    user_msg_t *pum = NULL;

    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("TLED: get_mpf(SMALL) err=%d\n", er);
        return er;
    }
    memset(pum, 0, sizeof(user_msg_t));
    pum->msgid  = MSGID_TLED_RES;
    pum->srctsk = TSKID_TLED;
    pum->dsttsk = dsttsk;      /* 応答先（要求元タスクID） */
    pum->result = (UH)result;
    pum->mpfid  = MPFID_SMALL;

    er = tk_snd_mbx(MBXID_TAPP, (T_MSG *)pum); /* TAPPで受けるなら MBXID_TAPP へ */
    if (er != E_OK) {
        APP_ERR_PRINT("TLED: snd_mbx err=%d\n", er);
        tk_rel_mpf(pum->mpfid, pum);
    }
    return er;
}

/* 初期化 */
static void init_task_tled(void)
{
    memset(&s_led, 0x00, sizeof(s_led));

    /* 既定は全消灯 */
    for (int i = 0; i < TLED_NUM; i++) {
        led_apply_pattern((tled_id_t)i, TLED_PAT_OFF, 0);
    }
}

/* タスク本体 */
EXPORT void task_tled(INT stacd, void *exinf)
{
    APP_PRINT("[TLED started]\n");
    init_task_tled();   // 全消灯などの初期化（既存関数）

    while (1) {
        ER er;
        user_msg_t *pum = NULL;

        // 100ms待ちでメッセージ受信。タイムアウト＝1Tick経過
        er = tk_rcv_mbx(MBXID_TLED, (T_MSG **)&pum, TLED_TICK_MS);

        if (er == E_OK && pum) {
            // ---- 受信した要求を処理 ----
            if (pum->msgid == MSGID_TLED_REQ) {
                const msg_led_req_t *req = (const msg_led_req_t *)&pum->pyload;

                if (req->led < TLED_NUM && req->pattern <= TLED_PAT_BLINK_SLOW) {
                    W count = (req->pattern == TLED_PAT_BLINK_FAST || req->pattern == TLED_PAT_BLINK_SLOW)
                              ? req->blink_count
                              : 0;
                    led_apply_pattern((tled_id_t)req->led, (tled_pattern_t)req->pattern, count);
                    // 応答（同期用）
                    (void)send_led_res(TRUE, pum->srctsk);
                } else {
                    (void)send_led_res(E_PAR, pum->srctsk);
                }
            } else {
                // 想定外メッセージは無視（必要ならログ）
                // APP_ERR_PRINT("TLED: unexpected msgid=%d\n", pum->msgid);
            }

            // ブロック解放
            (void)tk_rel_mpf(pum->mpfid, pum);

            // 受信できたループではTickは回さず次ループへ（即応優先）
            continue;
        }
        else if (er == E_TMOUT) {
            // ---- 100ms経過 → ステートマシンを1Tick進める ----
            led_tick();
            continue;
        }
        else {
            // 受信エラー（MBX未生成など） → 必要に応じて短い待ち・再試行
            APP_ERR_PRINT("TLED: rcv_mbx err=%d\n", er);
            tk_dly_tsk(10);
            continue;
        }
    }
}
