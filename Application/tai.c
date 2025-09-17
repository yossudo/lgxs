/**
 * TAIタスク
 *
 * 受け取った 16次元特徴に対してAI推論を行い、結果と可視化用128binスペクトルをTAPPへ返す
 */

#include "tai.h"

/* 簡易ダミー推論：featの総和でラベル判定 */
static int run_inference_feat(const float *feat16, float *score_out)
{
    float sum = 0.0f;
    for (int i = 0; i < FEAT_DIM; i++) sum += feat16[i];
    int res = (sum > 50.0f) ? 1 : 0;
    if (score_out) *score_out = res ? 0.9f : 0.1f;
    return res;
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

            tk_snd_mbx(MBXID_TAPP, (T_MSG *)pres);

            APP_PRINT("[TAI] inference done: result=%d score=%.3f\n", result, score);
        } else {
            APP_ERR_PRINT("[TAI] unexpected msgid=%d\n", pum->msgid);
        }

        tk_rel_mpf(pum->mpfid, pum);
    }
}
