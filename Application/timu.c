/**
 * TIMUタスク
 *
 * IMU(加速度、ジャイロ、地磁気センサ)から加速度を取得
 *
 * @file
 *
 * @note IMUはTDK InvenSense MPU-9250を使用
 * @note 100Hzでサンプリング
 * @note 1024個収集したら上位タスク(TAPP)へ通知
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#include "timu.h"

// 定数
#define TIMU_DEVNAME    "htiica"            // I2C device name
#define IMU_ADDR        0x68                // I2C devide address
#define CYC_PERIOD_MS 10                    // 100Hz = 10ms周期

// MPU9250 レジスタ
#define REG_ACCEL_XOUT_H  0x3B
#define REG_TEMP_OUT_H    0x41
#define REG_GYRO_XOUT_H   0x43

// MPU9250 scale factors
#define ACCEL_SENS_2G       (16384.0f)      // LSB/G
#define GYRO_SENS_250DPS    (131.0f)        // LSB/dps

// MPU9250データ構造体
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
LOCAL void init_task_timu(void);
LOCAL void init_gpt(void);
EXPORT void task_timu(INT stacd, void *exinf);
void gpt_handler(UINT intno);
LOCAL ER read2(ID dd, UB addr, UH *out);
LOCAL ER read_mpu9250(ID dd, mpu9250_data_t *pmd);
LOCAL void check_accel_config(ID dd);
LOCAL ER send_imu_ind(INT result, msg_imu_ind_t *pmir);


// グローバル変数
LOCAL ID i2cdd;                             // Device descriptor


/**
 * タスク初期化
 *
 * タスクの初期化
 * @param なし
 * @return なし
 */
LOCAL void init_task_timu(void)
{

    // GPTタイマを初期化
    init_gpt();

    // I2Cデバドラを初期化
    i2cdd = tk_opn_dev((UB*)TIMU_DEVNAME, TD_UPDATE);
    if (i2cdd < E_OK) {
        APP_ERR_PRINT("I3C-I2C Device open failed\n");
        return;
    }
    // ACCEL_CONFIGの確認
    check_accel_config(i2cdd);


    return;

}


/**
 * GPTタイマ初期化
 *
 * GPTタイマを初期化
 * @param なし
 * @return なし
 * @note GPTタイマの詳細設定はe2studioのFSP Cofigurationで行うこと
 */
LOCAL void init_gpt(void)
{

    fsp_err_t err;


    // GPT割り込みハンドラを登録
    T_DINT dint = { .intatr = TA_HLNG, .inthdr = (FP)gpt_handler };
    tk_def_int(GPT_INTNO, &dint);

    // GPTタイマをオープン
    err = R_GPT_Open (g_timer0.p_ctrl, g_timer0.p_cfg);
    if (FSP_SUCCESS != err) {
        /* ここでエラー処理 */
        return;
    }

    // タイマ起動（10ms周期=100Hz)
    err = R_GPT_Start(g_timer0.p_ctrl);
    if (FSP_SUCCESS != err) {
        /* ここでエラー処理 */
        return;
    }

    return;
}


/**
 * タスクメイン
 *
 * タスクのメイン処理
 * @param[in] stacd タスク起動時の開始コード
 * @param[in] exinf タスク起動時の拡張情報
 * @return なし
 */
EXPORT void task_timu(INT stacd, void *exinf) {

    static msg_imu_ind_t mir;
    static mpu9250_data_t md;
    int read_index;

    APP_PRINT("[TIMU started]\n");

    // タスク初期化
    init_task_timu();

    // グルグル．．．
    read_index = 0;
    while( 1 ) {

        // 先頭データならば．．．
        if (read_index == 0) {
            // 時刻を取得
            SYSTIM start;
            // メッセージ送信バッファをクリア
            memset(&mir, 0x00, sizeof(mir));
            tk_get_tim(&start);
            mir.tim = start;
        }

        // タイマから叩き起こされるのを待つ
        tk_slp_tsk(TMO_FEVR);

        // MPU-9250データを取得
        memset(&md, 0x00 ,sizeof(md));
        ER er = read_mpu9250(i2cdd, &md);
        if (er != E_OK) {
            APP_ERR_PRINT("IMU Read error: %d\n", er);
            continue;
        }

        // 加速度zを保存
        mir.accz[read_index] = md.az;

        read_index++;

        // IMU最大レコードを超えたら．．．
        if (read_index >= IMU_REC_MAX) {
            // メッセージ送信
            send_imu_ind(TRUE, &mir);
            // インデックスをゼロに戻す
            read_index = 0;
        }

    }

    return;
}


/**
 * GPT割り込みハンドラ
 *
 * 周期タイマの割り込みハンドラ
 * @param[in] intno
 * @return なし
 * @note GPTタイマの詳細設定はFSP Cofigurationで行うこと
 */
void gpt_handler(UINT intno)
{

    volatile R_GPT0_Type *gpt = R_GPT0;   // 使用チャンネルに合わせて
    uint32_t st = gpt->GTST;               // ステータス読取

    // 周期終端（オーバーフロー）だけ扱う
    if (st & R_GPT0_GTST_TCFPO_Msk) {

        // GPT本体のフラグクリア
        // そのビットだけを0にする（他ビットは保持）
        gpt->GTST = st & ~R_GPT0_GTST_TCFPO_Msk;

        // TIMUを叩き起こしてIMUからデータを取得させる
        tk_wup_tsk(TSKID_TIMU);

    }

    // ICU側のIRQ保留クリア
     R_BSP_IrqStatusClear(GPT_INTNO);

    // 割り込みハンドラを抜けるときのおまじない
    tk_ret_int();

}


/**
 * 2バイト読み込み
 *
 * MPU-9250の指定したレジスタから2バイト取得(I2C)
 * @param[in] dd デバイスディスクプリタ
 * @param[in] addr レジスタアドレス
 * @param[out] out 取得データへのポインタ
 * @return 処理結果
 * @retval E_OK 成功
 * @retval !E_OK エラー(デバドラのエラー値)
 */
LOCAL ER read2(ID dd, UB addr, UH *out)
{
    UB buf[2];
    SZ asize;
    ER er;

    // アドレスを指定
    er = tk_swri_dev(dd, IMU_ADDR, &addr, 1, &asize);
    if (er != E_OK) return er;

    // 2バイト取得
    er = tk_srea_dev(dd, IMU_ADDR, buf, 2, &asize);
    if (er != E_OK) return er;

    *out = (UH)((buf[0] << 8) | buf[1]);
    return E_OK;
}


/**
 * センサ出力を読み込み
 *
 * MPU-9250のレジスタから加速度、ジャイロ、温度値を取得
 * @param[in] dd デバイスディスクプリタ
 * @param[out] *pmd MPU-9250データ構造体へのポインタ
 * @return 処理結果
 * @retval E_OK 成功
 * @retval E_SYS エラー
 */
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

    return E_OK;
}


/**
 * 加速度センサ設定の確認
 *
 * 加速度センサ設定を表示(uart出力)
 * @param[in] dd デバイスディスクプリタ
 * @return なし
 */
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

    return;

}


/**
 * MSGID_IMU_INDを送信
 *
 * 収集した加速度データをtappへ送信
 * @param[in] result 結果
 * @param[in] pdata  収集した加速度データへのポインタ
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_imu_ind(INT result, msg_imu_ind_t *pdata)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_imu_ind_t *pmir = NULL;

    // 固定長メモリを取得
    er = tk_get_mpf(MPFID_LARGE, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    // メッセージ構造体を作成
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TIMU_IND;
    pum->srctsk = TSKID_TIMU;
    pum->dsttsk = TSKID_TAPP;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_LARGE;

    pmir = (msg_imu_ind_t *)&pum->pyload;
    memcpy(pmir, pdata, sizeof(msg_imu_ind_t));

    // メッセージ送信
    er = tk_snd_mbx( MBXID_TAPP, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


