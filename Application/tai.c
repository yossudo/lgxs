/**
 * TAIタスク
 *
 * 受け取った 16次元特徴に対してAI推論を行い、結果と可視化用128binスペクトルをTAPPへ返す
 */

#include "tai.h"

// 判定スコアの滑らかさ（放置で0に張り付くの防止＆チラつき抑制）
#ifndef TAI_SCORE_ALPHA
#define TAI_SCORE_ALPHA 0.25f   // 0<α<=1  大きいほど追従速い
#endif
#ifndef TAI_SCORE_FLOOR
#define TAI_SCORE_FLOOR 0.08f   // スコアの下限フロア（放置で0張り付き防止）
#endif



static inline float relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}


static inline float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}


/* 推論　*/
static int run_inference_feat(const float feat[FEAT_DIM], float *score_out)
{

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
}


/* タスクメイン処理 */
EXPORT void task_tai(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;

    ER ercd;
    user_msg_t *pum = NULL;

    APP_PRINT("[TAI started]\n");

    // グルグル、、、
    while (1) {

        // メッセージ受信待ち
        ercd = tk_rcv_mbx(MBXID_TAI, (T_MSG **)&pum, TMO_FEVR);
        if (ercd != E_OK) {
            APP_ERR_PRINT("[TAI] rcv_mbx err=%d\n", ercd);
            continue;
        }

        // MSGID_TAI_REQ受信ならば、、、
        if (pum->msgid == MSGID_TAI_REQ) {

            const msg_ai_req_t *preq = (const msg_ai_req_t *)&pum->pyload;

            // AI推論
            float score = 0.0f;
            int result = run_inference_feat(preq->feat, &score);

            // 結果を返却
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
        }
        else {
            APP_ERR_PRINT("[TAI] unexpected msgid=%d\n", pum->msgid);
        }

        tk_rel_mpf(pum->mpfid, pum);
    }
}
