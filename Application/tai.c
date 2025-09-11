/**
 * TAIタスク
 *
 * 加速度データから信号処理、AI危険度判定を行い、結果をTAPPへ送信する
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */

/*
 * tai.c
 * TAIタスク: FFT解析とAI推論
 */

#include "tai.h"

/* === 定義 === */
#define FFT_SIZE IMU_REC_MAX
#define FFT_OUT_SIZE (FFT_SIZE / 2)

/* === FFT作業バッファ === */
static float32_t fft_input[FFT_SIZE];
static float32_t fft_output[FFT_SIZE];      // 複素数対で出力
static float32_t fft_magnitude[FFT_OUT_SIZE];

static arm_rfft_fast_instance_f32 rfft_instance;

/* === 内部関数 === */
static void perform_fft(const UH *accz, float32_t *magnitude)
{
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_input[i] = (float32_t)accz[i];
    }

    // FFT実行
    arm_rfft_fast_f32(&rfft_instance, fft_input, fft_output, 0);

    // 複素数出力 → 振幅スペクトルに変換
    for (int i = 0; i < FFT_OUT_SIZE; i++) {
        float32_t real = fft_output[2 * i];
        float32_t imag = fft_output[2 * i + 1];
        magnitude[i] = sqrtf(real * real + imag * imag);
    }
}

/* === AI推論ダミー関数 === */
static int run_inference(const float32_t *magnitude)
{
    // TODO: 学習済みモデルへ置き換え
    // 現状は「ピークが一定閾値を超えたら1、それ以外は0」とする簡易判定
    float32_t max_val = 0.0f;
    uint32_t max_idx = 0;
    arm_max_f32(magnitude, FFT_OUT_SIZE, &max_val, &max_idx);

    if (max_val > 5000.0f) {
        return 1;   // 異常傾向あり
    } else {
        return 0;   // 正常
    }
}

/* === タスク本体 === */
EXPORT void task_tai(INT stacd, void *exinf)
{
    ER ercd;
    user_msg_t *pum = NULL;

    // FFT初期化
    arm_rfft_fast_init_f32(&rfft_instance, FFT_SIZE);

    APP_PRINT("[TAI started]\n");

    while (1) {
        // メッセージ受信（TAPPからの要求待ち）
        ercd = tk_rcv_mbx(MBXID_TAI, (T_MSG **)&pum, TMO_FEVR);
        if (ercd != E_OK) {
            APP_ERR_PRINT("[TAI] rcv_mbx err=%d\n", ercd);
            continue;
        }

        if (pum->msgid == MSGID_TAI_REQ) {
            const msg_imu_ind_t *preq = (const msg_imu_ind_t *)&pum->pyload;

            // FFT計算
            perform_fft(preq->accz, fft_magnitude);

#if 0
            // debug
            APP_PRINT("%llu", SYSTIM_TO_UD(preq->tim));
            for (int i = 0; i < FFT_OUT_SIZE; i++) {
                char valstr[16];
                sprintf(valstr, ",%.2f", fft_magnitude[i]);
                APP_PRINT(valstr);
            }
            APP_PRINT("\n");
#endif

            // AI推論
            int result = run_inference(fft_magnitude);


            // 応答メッセージ作成
            user_msg_t *pres;
            ercd = tk_get_mpf(MPFID_MEDIUM, (void **)&pres, TMO_FEVR);
            if (ercd != E_OK) {
                APP_ERR_PRINT("[TAI] get_mpf err=%d\n", ercd);
                tk_rel_mpf(pum->mpfid, pum);
                continue;
            }

            pres->msgid = MSGID_TAI_RES;
            pres->srctsk = TSKID_TAI;
            pres->dsttsk = TSKID_TAPP;
            pres->result = result;
            pres->mpfid = MPFID_MEDIUM;
            msg_ai_res_t *pout = (msg_ai_res_t *)&pres->pyload;
            pout->tim = preq->tim;

            memcpy(pout->spectrum, fft_magnitude, sizeof(float32_t) * FFT_OUT_SIZE);

            // TAPPへ応答送信
            tk_snd_mbx(MBXID_TAPP, (T_MSG *)pres);

            APP_PRINT("[TAI] inference done: result=%d\n", result);
        }

        // 要求メモリ返却
        tk_rel_mpf(pum->mpfid, pum);
    }
}

