/**
 * TAIタスク
 *
 * 受け取った 16次元特徴に対してAI推論を行い、結果と可視化用128binスペクトルをTAPPへ返す
 */

#include "tai.h"

static float clamp01(float x){ return x<0.f?0.f:(x>1.f?1.f:x); }
static inline float clamp01f(float x){ return (x<0.f)?0.f:(x>1.f?1.f:x); }

// 判定スコアの滑らかさ（放置で0に張り付くの防止＆チラつき抑制）
#ifndef TAI_SCORE_ALPHA
#define TAI_SCORE_ALPHA 0.25f   // 0<α<=1  大きいほど追従速い
#endif
#ifndef TAI_SCORE_FLOOR
#define TAI_SCORE_FLOOR 0.08f   // スコアの下限フロア（放置で0張り付き防止）
#endif


// N=128 前提の簡易メディアン（コピーして nth_element 相当）
// メモリが厳しければ、P2Pメディアンや分位点推定でもOK
static float median128(const float *x)
{
    float w[128];
    for (int i=0;i<128;i++) w[i]=x[i];
    // 簡易の挿入ソート（N固定・一時的にOK。計算量が気になるなら別実装へ）
    for (int i=1;i<128;i++){
        float key=w[i]; int j=i-1;
        while (j>=0 && w[j]>key){ w[j+1]=w[j]; j--; }
        w[j+1]=key;
    }
    return 0.5f*(w[63]+w[64]);
}

static inline float relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

static inline float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

static int run_inference_feat(const float feat[FEAT_DIM], float *score_out)
{
#if 1
    float hidden[HIDDEN_DIM];

    // --- 1層目 (feat → hidden) ---
    for (int j = 0; j < HIDDEN_DIM; j++) {
        float sum = mlp_b0[j];
        for (int i = 0; i < FEAT_DIM; i++) {
            sum += feat[i] * mlp_w0[i * HIDDEN_DIM + j];
        }
        hidden[j] = relu(sum);
    }

    // --- 2層目 (hidden → 出力) ---
    float sum = mlp_b1[0];
    for (int j = 0; j < HIDDEN_DIM; j++) {
        sum += hidden[j] * mlp_w1[j];
    }
    float y = sigmoid(sum);

    *score_out = y;
    return (y >= 0.5f) ? 1 : 0;  // しきい値は運用で調整可能
#else
    // feat[15] : ピークSNRのEMA [dB]
    // feat[13] : ピーク/床 log10(パワー比) → dB化で×10
    // feat[6]  : スペクトルフラットネス(0..1)

    const float snr_db   = feat[15];
    const float prom_db  = 10.0f * feat[13];
    const float flatness = feat[6];

    // --- 正規化（レンジを優しめに） ---
    // SNR: 0dB→0, 18dB→1  （以前より低SNRでも0に振り切れにくい）
    float s_snr = (snr_db - 0.0f) / 18.0f;
    s_snr = clamp01f(s_snr);

    // 顕著度: 1dB→0, 12dB→1
    float s_prom = (prom_db - 1.0f) / 11.0f;
    s_prom = clamp01f(s_prom);

    // 合成（同等重み）
    float s = 0.5f * s_snr + 0.5f * s_prom;

    // 白色っぽいスペクトルは減点。ただし緩やかに（最大30%）
    if (flatness > 0.55f) {
        float pen = (flatness - 0.55f) / (0.90f - 0.55f);  // 0.55→0, 0.9→1
        if (pen < 0.f) pen = 0.f; if (pen > 1.f) pen = 1.f;
        s *= (1.0f - 0.30f * pen);
    }

    // --- スコアのEMA平滑化＋下限フロア ---
    static int   inited = 0;
    static float s_ema  = 0.2f; // 初期バイアス（放置で0張り付き防止）
    if (!inited) { inited = 1; }

    s = TAI_SCORE_ALPHA * s + (1.0f - TAI_SCORE_ALPHA) * s_ema;
    s_ema = s;

    if (s < TAI_SCORE_FLOOR) s = TAI_SCORE_FLOOR;  // 下限フロア

    *score_out = clamp01f(s);

    // しきい値（初期値）。このままでも「放置は0.1〜0.3」「振動投入で0.6超」が狙い。
    const float TH = 0.55f;

    return (*score_out >= TH) ? 1 : 0;
#endif
}


EXPORT void task_tai(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;

    ER ercd;
    user_msg_t *pum = NULL;

    APP_PRINT("[TAI started]\n");

    while (1) {
        ercd = tk_rcv_mbx(MBXID_TAI, (T_MSG **)&pum, TMO_FEVR);
        if (ercd != E_OK) {
            APP_ERR_PRINT("[TAI] rcv_mbx err=%d\n", ercd);
            continue;
        }

        if (pum->msgid == MSGID_TAI_REQ) {
            const msg_ai_req_t *preq = (const msg_ai_req_t *)&pum->pyload;

            float score = 0.0f;
            int result = run_inference_feat(preq->feat, &score);

            user_msg_t *pres = NULL;
            ercd = tk_get_mpf(MPFID_MEDIUM, (void **)&pres, TMO_FEVR);
            if (ercd != E_OK) {
                APP_ERR_PRINT("[TAI] get_mpf err=%d\n", ercd);
                tk_rel_mpf(pum->mpfid, pum);
                continue;
            }

            memset(pres, 0, sizeof(*pres));
            pres->msgid  = MSGID_TAI_RES;
            pres->srctsk = TSKID_TAI;
            pres->dsttsk = TSKID_TAPP;
            pres->result = result;
            pres->mpfid  = MPFID_MEDIUM;

            msg_ai_res_t *pout = (msg_ai_res_t *)&pres->pyload;
            pout->tim = preq->tim;
            memcpy(pout->spectrum, preq->spectrum, sizeof(float32_t) * SPEC_DIM);
            pout->label = (int8_t)result;
            pout->score = score;
            pout->conf_anom = score;
            pout->conf_norm = 1.0f - score;
            pout->bin_hz = preq->bin_hz;
            for (int i=0;i<FEAT_DIM;i++)
                pout->feat[i] = preq->feat[i];


            tk_snd_mbx(MBXID_TAPP, (T_MSG *)pres);

            int32_t qscore = (int32_t)(score*1000.0f);

            APP_PRINT("[TAI] inference done: result=%d score=%d.%03d\n", result, (int)(qscore/1000), (int)abs(qscore%1000));
        } else {
            APP_ERR_PRINT("[TAI] unexpected msgid=%d\n", pum->msgid);
        }

        tk_rel_mpf(pum->mpfid, pum);
    }
}
