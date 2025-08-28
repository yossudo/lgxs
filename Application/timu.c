/*
 * timu.c
 *
 *  Created on: 2025/07/05
 *      Author: yoshi
 */
#include "timu.h"

#define TIMU_DEVNAME   "htiica"
#define IMU_ADDR       0x68
#define TIMU_INTERVAL_MS ((TMO)(10 - 5))  // 100Hz = 10ms周期
#define CYC_PERIOD_MS 10  // 100Hz = 10ms周期

// MPU9250 scale factors
#define ACCEL_SENS_2G   (16384.0f)   // LSB/G
#define GYRO_SENS_250DPS (131.0f)    // LSB/dps

LOCAL ID i2cdd;  // Device descriptor

// MPU9250 レジスタ
#define REG_ACCEL_XOUT_H  0x3B
#define REG_TEMP_OUT_H    0x41
#define REG_GYRO_XOUT_H   0x43

typedef struct {
    UH ax;
    UH ay;
    UH az;
    UH gx;
    UH gy;
    UH gz;
    UH temp;
} mpu9250_data_t;



// 関数プロトタイプ
LOCAL ER read2(ID dd, UB addr, UH *out);
LOCAL ER read_mpu9250(ID dd, mpu9250_data_t *pmd);
LOCAL ER send_imu_req(INT result, msg_imu_req_t *pmir);
LOCAL void init_task_timu(void);
EXPORT void task_timu(INT stacd, void *exinf);




// 補助関数：2バイト読み込み
LOCAL ER read2(ID dd, UB addr, UH *out) {
    UB buf[2];
    SZ asize;
    ER er;

    er = tk_swri_dev(dd, IMU_ADDR, &addr, 1, &asize);
    if (er != E_OK) return er;

    er = tk_srea_dev(dd, IMU_ADDR, buf, 2, &asize);
    if (er != E_OK) return er;

    *out = (UH)((buf[0] << 8) | buf[1]);
    return E_OK;
}


// 補助関数：センサ出力を読み込む
LOCAL ER read_mpu9250(ID dd, mpu9250_data_t *pmd) {
    UH ax, ay, az;
    UH gx, gy, gz;
    UH temp;

    if (read2(dd, REG_ACCEL_XOUT_H, &ax) != E_OK) return E_SYS;
    if (read2(dd, REG_ACCEL_XOUT_H + 2, &ay) != E_OK) return E_SYS;
    if (read2(dd, REG_ACCEL_XOUT_H + 4, &az) != E_OK) return E_SYS;

    if (read2(dd, REG_GYRO_XOUT_H, &gx) != E_OK) return E_SYS;
    if (read2(dd, REG_GYRO_XOUT_H + 2, &gy) != E_OK) return E_SYS;
    if (read2(dd, REG_GYRO_XOUT_H + 4, &gz) != E_OK) return E_SYS;

    if (read2(dd, REG_TEMP_OUT_H, &temp) != E_OK) return E_SYS;

    pmd->ax = ax;
    pmd->ay = ay;
    pmd->az = az;
    pmd->gx = gx;
    pmd->gx = gx;
    pmd->gx = gx;
    pmd->temp = temp;

    #if 0
    // 符号付きへキャスト
    H sax = (H)ax;
    H say = (H)ay;
    H saz = (H)az;
    H sgx = (H)gx;
    H sgy = (H)gy;
    H sgz = (H)gz;
    H stemp = (H)temp;

    // 単位変換
    float acc_scale = 2.0f / 32768.0f;   // ±2G
    float gyro_scale = 250.0f / 32768.0f; // ±250dps

    float fax = (float)sax * acc_scale;
    float fay = (float)say * acc_scale;
    float faz = (float)saz * acc_scale;

    float fgx = (float)sgx * gyro_scale;
    float fgy = (float)sgy * gyro_scale;
    float fgz = (float)sgz * gyro_scale;

    float ft = ((float)stemp / 321.0f) + 21.0f;

    // RTT printf は float 非対応なので一旦バッファに文字列を作る
    static char msg[256];
    sprintf(msg, "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f\n",
                  fax, fay, faz,
                  fgx, fgy, fgz,
                  ft);

    APP_PRINT("%s", msg);
#endif

    return E_OK;
}

LOCAL void check_accel_config(ID dd)
{
    UB accel_config = 0;
    SZ asize;
    UB addr = 0x1C;
    ER er;

    er = tk_swri_dev(dd, IMU_ADDR, &addr, 1, &asize);
    if (er != E_OK)
    {
        APP_ERR_PRINT("Failed to write ACCEL_CONFIG addr\n");
        return;
    }

    er = tk_srea_dev(dd, IMU_ADDR, &accel_config, 1, &asize);
    if (er != E_OK)
    {
        APP_ERR_PRINT("Failed to read ACCEL_CONFIG\n");
        return;
    }

    UB fs_sel = (accel_config >> 3) & 0x03;

    APP_PRINT("ACCEL_CONFIG = 0x%02X -> FS_SEL = %d -> ", accel_config, fs_sel);
    switch (fs_sel)
    {
        case 0: APP_PRINT("±2G\n"); break;
        case 1: APP_PRINT("±4G\n"); break;
        case 2: APP_PRINT("±8G\n"); break;
        case 3: APP_PRINT("±16G\n"); break;
        default: APP_PRINT("Unknown range\n"); break;
    }
}



LOCAL ER send_imu_req(INT result, msg_imu_req_t *pdata)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_imu_req_t *pmir = NULL;

    er = tk_get_mpf(MPFID_LARGE, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TIMU_REQ;
    pum->srctsk = TSKID_TIMU;
    pum->dsttsk = TSKID_TAPP;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_LARGE;

    pmir = (msg_imu_req_t *)&pum->pyload;
    memcpy(pmir, pdata, sizeof(msg_imu_req_t));

    er = tk_snd_mbx( MBXID_TAPP, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


void gpt_handler(UINT intno)
{

    volatile R_GPT0_Type * gpt = R_GPT0;   // ←使用チャンネルに合わせて
    uint32_t st = gpt->GTST;               // ステータス読取

    // 周期終端（オーバーフロー）だけ扱う
    if (st & R_GPT0_GTST_TCFPO_Msk) {

        // ---- ① GPT本体のフラグクリア ----
        // そのビットだけを0にする（他ビットは保持）
        gpt->GTST = st & ~R_GPT0_GTST_TCFPO_Msk;

        tk_wup_tsk(TSKID_TIMU);

    }

    // ---- ② ICU側のIRQ保留クリア ----
     R_BSP_IrqStatusClear(GPT_INTNO);

    // GPTの割込みフラグはFSPで適切に処理されるため、ここでの個別クリアは不要
    tk_ret_int();
}



LOCAL void init_gpt()
{


    T_DINT dint = { .intatr = TA_HLNG, .inthdr = (FP)gpt_handler };
    tk_def_int(GPT_INTNO, &dint);

    fsp_err_t err;
    // GPTオープン＆開始
    err = R_GPT_Open (g_timer0.p_ctrl, g_timer0.p_cfg);
    if (FSP_SUCCESS != err) { /* ここでエラー処理 */ }

    err = R_GPT_Start(g_timer0.p_ctrl);
    if (FSP_SUCCESS != err) { /* ここでエラー処理 */ }

    return;
}


// タスク初期化
LOCAL void init_task_timu(void)
{

    init_gpt();

    i2cdd = tk_opn_dev((UB*)TIMU_DEVNAME, TD_UPDATE);
    if (i2cdd < E_OK) {
        APP_ERR_PRINT("I3C-I2C Device open failed\n");
        return;
    }
    // ACCEL_CONFIGの確認
    check_accel_config(i2cdd);



    return;

}

// タスクメイン
EXPORT void task_timu(INT stacd, void *exinf) {

    static msg_imu_req_t mir;
    static mpu9250_data_t md;
    int read_index;
    SYSTIM start, end;

    UINT pat;

    APP_PRINT("[TIMU started]\n");

    // タスク初期化
    init_task_timu();

#if 1
    read_index = 0;
    memset(&mir, 0x00, sizeof(mir));
    while( 1 ) {

        tk_slp_tsk(TMO_FEVR);

        tk_get_tim(&start);
#if 1
        memset(&md, 0x00 ,sizeof(md));
        ER er = read_mpu9250(i2cdd, &md);
        if (er != E_OK) {
            APP_ERR_PRINT("IMU Read error: %d\n", er);
        }
#endif
        mir.tim = start;
        mir.accz[read_index] = md.az;

        read_index++;
        if (read_index >= 1024) {
            read_index = 0;
            send_imu_req(TRUE, &mir);

        }

    }
#else
    TMO tmo = TIMU_INTERVAL_MS * 1000;
    //TMO tmo = TMO_FEVR;
    read_index = 0;
    memset(&mir, 0x00, sizeof(mir));

    // メインループ
    while(1){

        UW a = millis32();

        user_msg_t *pum = NULL;
        ER er = tk_rcv_mbx( MBXID_TIMU, (T_MSG **)&pum, tmo );
        if (er == E_TMOUT) {
            UW b = millis32();

            UW c = b - a;

            //APP_PRINT("tmo rcv_mbx:%d\n", er);
            tk_get_tim(&start);
 #if 1
            memset(&md, 0x00 ,sizeof(md));
            er = read_mpu9250(i2cdd, &md);
            if (er != E_OK) {
                APP_ERR_PRINT("IMU Read error: %d\n", er);
            }
#endif

            mir.tim = start;
            mir.accz[read_index] = md.az;

            read_index++;
            if (read_index >= 1024) {
                read_index = 0;
                send_imu_req(TRUE, &mir);
            }

            ///////////////
            tk_get_tim(&end);
            TMO diff = (TMO)(SYSTIM_TO_UD(end) - SYSTIM_TO_UD(start));
            if (diff < TIMU_INTERVAL_MS) {
                tmo = TIMU_INTERVAL_MS - diff;
            } else {
                APP_ERR_PRINT("Sampling delay overrun (%ldms)\n", diff);
                tmo = TIMU_INTERVAL_MS; // 仕切り直し？
            }
        }
        else if (er == E_OK) {

            er = tk_rel_mpf(pum->mpfid, pum);
            if (er != E_OK) {
                APP_ERR_PRINT("error rel_mpf:%d\n", er);
                //continue;
            }


        }
        else {
            APP_ERR_PRINT("error rcv_mbx:%d\n", er);
            continue;
        }

    }

#endif

    return;
}
