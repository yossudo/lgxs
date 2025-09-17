/**
 * TAPPタスク
 *
 * アプリケーションタスク
 *
 * @file
 *
 * @note 周辺タスクをコントロールし、アプリケーションを実現する
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#include "tapp.h"
#include "tled.h"

#include <math.h>
#include "arm_math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* サンプリング条件 */
#ifndef FFT_N
#define FS_HZ          100.0f
#define FFT_N          1024
#define BIN_OUT        (FFT_N/2)      /* 512 */
#define BIN128         128
#define BIN_HZ         (FS_HZ*0.5f / (BIN128-1))
#endif

/* RFFTインスタンスとHann窓バッファ（ここで定義しておく） */
static arm_rfft_fast_instance_f32 Sfft;
static float g_hann[FFT_N];

/* 窓生成＆RFFT初期化 */
static void tapp_make_hann(void){
    for (int i=0;i<FFT_N;i++){
        g_hann[i] = 0.5f*(1.0f - arm_cos_f32(2.0f*(float)M_PI*i/(FFT_N-1)));
    }
}
static void tapp_fft_init(void){
    tapp_make_hann();
    arm_rfft_fast_init_f32(&Sfft, FFT_N);
}

/* HPF(1次) パラメータ（fcは適宜調整可） */
#define HPF_FC_HZ      0.5f
#define HPF_A          (expf(-2.0f * M_PI * HPF_FC_HZ / FS_HZ))

/* Welch代替：時間EMAでのスペクトル平滑 */
#define USE_EMA        1
#define EMA_BETA       0.85f          /* 大きいほどなめらか */

/* ノイズ床（中央値）窓幅：奇数 */
#define MED_WIN        7

/* ---- HPF(1次) ---- */
typedef struct {
    float a;  /* 0<a<1 */
    float y1;
    float x1;
} hpf1_t;

static inline void hpf1_init(hpf1_t* f) {
    f->a = HPF_A;
    f->y1 = 0.0f;
    f->x1 = 0.0f;
}
static inline float hpf1_step(hpf1_t* f, float x) {
    float y = f->a * (f->y1 + x - f->x1);
    f->y1 = y; f->x1 = x;
    return y;
}

/* ---- LPF(2次Biquad)（任意・係数は後で最適化）---- */
/* fs=100Hz, fc=35Hz, Q=0.707 の係数例（双一次変換）: 必要時のみ有効化 */
#define USE_LPF        0
typedef struct {
    arm_biquad_casd_df1_inst_f32 S;
    float coeffs[5];
    float state[4];
} lpf2_t;

/* 係数は設計ツールで置き換えてOK。ここでは安全な「素通しに近い」仮係数 */
static inline void lpf2_init(lpf2_t* f) {
    /* b0, b1, b2, a1, a2 */
    f->coeffs[0]=1.0f; f->coeffs[1]=0.0f; f->coeffs[2]=0.0f; f->coeffs[3]=0.0f; f->coeffs[4]=0.0f;
    arm_biquad_cascade_df1_init_f32(&f->S, 1, f->coeffs, f->state);
}
static inline void lpf2_process(lpf2_t* f, float* x, uint32_t n) {
    arm_biquad_cascade_df1_f32(&f->S, x, x, n);
}

/* ---- 近傍中央値によるローカルノイズ床 ---- */
static void local_median_floor(const float* p, float* floor, int n, int w){
    int r = w/2;
    for(int i=0;i<n;i++){
        float tmp[17]; /* w<=17 想定 */
        int m=0;
        for(int j=i-r;j<=i+r;j++){
            int jj = (j<0)?0:((j>=n)?(n-1):j);
            tmp[m++] = p[jj];
        }
        for(int a=0;a<m-1;a++) for(int b=a+1;b<m;b++) { if(tmp[a]>tmp[b]){float t=tmp[a]; tmp[a]=tmp[b]; tmp[b]=t;} }
        floor[i]=tmp[m/2];
    }
}

/* ---- スペクトル重心などの補助 ---- */
static float spectral_centroid_hz(const float* p, int n){
    float num=0.0f, den=0.0f;
    for(int i=0;i<n;i++){ float f=i*BIN_HZ; num+=f*p[i]; den+=p[i]; }
    return (den>1e-12f)? (num/den) : 0.0f;
}
static float spectral_spread_hz(const float* p, int n, float centroid){
    float num=0.0f, den=0.0f;
    for(int i=0;i<n;i++){ float f=i*BIN_HZ; float d=f-centroid; num+=d*d*p[i]; den+=p[i]; }
    return (den>1e-12f)? sqrtf(num/den) : 0.0f;
}
static float spectral_rolloff_hz(const float* p, int n, float ratio){ /* ratio=0.85等 */
    float total=0.0f; for(int i=0;i<n;i++) total+=p[i];
    float th=total*ratio, acc=0.0f;
    for(int i=0;i<n;i++){ acc+=p[i]; if(acc>=th) return i*BIN_HZ; }
    return (n-1)*BIN_HZ;
}
static float spectral_flatness(const float* p, int n){ /* 幾何平均/算術平均 */
    float sum=0.0f, logsum=0.0f;
    for(int i=0;i<n;i++){ float x=fmaxf(p[i], 1e-18f); sum+=x; logsum+=logf(x); }
    float am = sum/n;
    float gm = expf(logsum/n);
    return (am>1e-18f)? (gm/am) : 0.0f;
}

/* ---- ピーク（最大SNR）抽出 ---- */
typedef struct {
    int   idx;
    float f_hz;
    float snr_db;
    float width_hz;
} peak_t;

static peak_t pick_peak_snr(const float* power, const float* floor, int n){
    int imax=1; float best=0.0f;
    for(int i=1;i<n-1;i++){
        float snr = power[i] / (floor[i] + 1e-12f);
        if (snr > best){ best=snr; imax=i; }
    }
    /* 半値幅 */
    int l=imax, r=imax; float half = power[imax]*0.5f;
    while(l>1   && power[l] > half) l--;
    while(r<n-2 && power[r] > half) r++;
    peak_t pk;
    pk.idx = imax;
    pk.f_hz = imax*BIN_HZ;
    pk.snr_db = 10.0f*log10f(best + 1e-12f);
    pk.width_hz = (float)(r - l)*BIN_HZ;
    return pk;
}


// 関数プロトタイプ
LOCAL void init_task_tapp(void);
EXPORT void task_tapp(INT stacd, void *exinf);
LOCAL ER send_ai_req(INT result, msg_ai_req_t *data);
LOCAL ER send_net_req(INT result, SYSTIM tim, const float *spectrum, size_t spectrum_len);
LOCAL ER send_led_req(INT result, UB led, UB pattern, W blink_count);

static hpf1_t   g_hpf;
#if USE_LPF
static lpf2_t   g_lpf;
#endif
static float    g_psd_ema[BIN128];
static int      g_ema_init = 0;

/**
 * タスク初期化
 *
 * タスクの初期化
 * @param なし
 * @return なし
 */
LOCAL void init_task_tapp(void)
{

    // 周辺タスクが上がるまでちょっと待つ
    tk_dly_tsk(100);

    send_led_req(TRUE, TLED_GREEN, TLED_PAT_ON, 0);

    tapp_fft_init();

    hpf1_init(&g_hpf);
   #if USE_LPF
       lpf2_init(&g_lpf);
   #endif
    for(int i=0;i<BIN128;i++) g_psd_ema[i]=0.0f;
    g_ema_init = 0;

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
EXPORT void task_tapp(INT stacd, void *exinf) {

    APP_PRINT("[TAPP started]\n");

    // タスク初期化
    init_task_tapp();

    // グルグル．．．
    ER er;
    user_msg_t *pum = NULL;
    msg_imu_ind_t *mir = NULL;
    while( 1 ) {
        // メッセージ受信待ち
        er = tk_rcv_mbx( MBXID_TAPP, (T_MSG **)&pum, TMO_FEVR );
        if (er != E_OK && er != E_TMOUT) {
            APP_ERR_PRINT("error rcv_mbx:%d\n", er);
            continue;
        }

        if (pum->msgid == MSGID_TIMU_IND) {


            mir = (msg_imu_ind_t *)&pum->pyload;

            APP_PRINT( "rcv_mbx TAPP:[%d]\n", pum->result );

            APP_PRINT("%llu - ", SYSTIM_TO_UD(mir->tim));
            for (int i =0; i < 16; i++) {
                APP_PRINT("%d ", mir->accz[i]);
            }
            APP_PRINT("\n");

            /* --- 1) HPF（必要ならLPF） → Hann窓 → RFFT → power512 --- */
            static float x[FFT_N];
            static float X[FFT_N];
            static float P512[BIN_OUT];

            for (int i=0;i<FFT_N;i++) {
                float v = (float)mir->accz[i];
                v = hpf1_step(&g_hpf, v);            /* HPF */
        #if USE_LPF
                x[i] = v;                            /* LPFは配列でまとめて処理 */
        #else
                x[i] = v;
        #endif
            }
        #if USE_LPF
            lpf2_process(&g_lpf, x, FFT_N);          /* 任意 */
        #endif

            for (int i=0;i<FFT_N;i++) x[i] *= g_hann[i];
            arm_rfft_fast_f32(&Sfft, x, X, 0);

            P512[0] = X[0]*X[0];
            for (int k=1;k<BIN_OUT-1;k++){
                float re = X[2*k], im = X[2*k+1];
                P512[k] = re*re + im*im;
            }
            P512[BIN_OUT-1] = X[1]*X[1];

            /* --- 2) 512→128 統合（パワー加算） --- */
            static float spec128[BIN128];
            for (int j=0;j<BIN128;j++){
                int k0 = j*4;
                spec128[j] = P512[k0+0] + P512[k0+1] + P512[k0+2] + P512[k0+3];
            }

            /* --- 3) 時間方向EMAで平滑化（視覚/判定ともに安定化） --- */
        #if USE_EMA
            if (!g_ema_init){
                for (int j=0;j<BIN128;j++) g_psd_ema[j] = spec128[j];
                g_ema_init = 1;
            } else {
                for (int j=0;j<BIN128;j++) g_psd_ema[j] = EMA_BETA*g_psd_ema[j] + (1.0f-EMA_BETA)*spec128[j];
            }
            const float* p128 = g_psd_ema;
        #else
            const float* p128 = spec128;
        #endif

            /* --- 4) ノイズ床（近傍中央値）→SNRベクトル --- */
            static float floor128[BIN128];
            local_median_floor(p128, floor128, BIN128, MED_WIN);

            /* --- 5) ピーク抽出＆特徴生成（16次元） --- */
            peak_t pk = pick_peak_snr(p128, floor128, BIN128);

            static float feat[FEAT_DIM] = {0};
            /* f0..2: ピーク情報 */
            feat[0]  = pk.f_hz;       /* 最大SNRピーク周波数 [Hz] */
            feat[1]  = pk.snr_db;     /* そのSNR [dB] */
            feat[2]  = pk.width_hz;   /* 半値幅推定 [Hz] */

            /* f3..5: スペクトル形状 */
            float sc = spectral_centroid_hz(p128, BIN128);
            feat[3]  = sc;                                  /* 重心周波数 */
            feat[4]  = spectral_spread_hz(p128, BIN128, sc);/* スプレッド */
            feat[5]  = spectral_rolloff_hz(p128, BIN128, 0.85f); /* 85%ロールオフ */

            /* f6: フラットネス（0..1） */
            feat[6]  = spectral_flatness(p128, BIN128);

            /* f7..11: 4Hz幅×5帯域のパワー合計（0-4,4-8,8-12,12-16,16-20Hz） */
            int bands[][2] = { {0, (int)roundf(4.0f/BIN_HZ)},
                               {(int)roundf(4.0f/BIN_HZ), (int)roundf(8.0f/BIN_HZ)},
                               {(int)roundf(8.0f/BIN_HZ), (int)roundf(12.0f/BIN_HZ)},
                               {(int)roundf(12.0f/BIN_HZ), (int)roundf(16.0f/BIN_HZ)},
                               {(int)roundf(16.0f/BIN_HZ), (int)roundf(20.0f/BIN_HZ)} };
            for (int b=0;b<5;b++){
                int i0=bands[b][0]; int i1=bands[b][1];
                i0 = (i0<0)?0:i0; i1 = (i1>BIN128)?BIN128:i1;
                float s=0.0f; for(int i=i0;i<i1;i++) s+=p128[i];
                feat[7+b] = log10f(s + 1.0f); /* 対数圧縮 */
            }

            /* f12: 低域/高域比（0-10Hz vs 10-25Hz） */
            int L0=0, L1=(int)roundf(10.0f/BIN_HZ);
            int H0=L1, H1=(int)roundf(25.0f/BIN_HZ);
            if (L1>BIN128) L1=BIN128;
            if (H1>BIN128) H1=BIN128;
            float slow=0.0f, shigh=0.0f;
            for(int i=L0;i<L1;i++) slow += p128[i];
            for(int i=H0;i<H1;i++) shigh+= p128[i];
            feat[12] = log10f((slow+1e-6f)/(shigh+1e-6f));

            /* f13: ピーク顕著度（ピーク/近傍床） */
            feat[13] = log10f( (p128[pk.idx] + 1e-9f) / (floor128[pk.idx] + 1e-9f) );

            /* f14,f15: ピーク周波数/ピークSNRのEMA（時間安定化） */
            static int   init_pf = 0;
            static float pf_ema  = 0.0f, snr_ema = 0.0f;
            if (!init_pf){ pf_ema = pk.f_hz; snr_ema = pk.snr_db; init_pf=1; }
            else{
                pf_ema  = 0.8f*pf_ema  + 0.2f*pk.f_hz;
                snr_ema = 0.8f*snr_ema + 0.2f*pk.snr_db;
            }
            feat[14] = pf_ema;
            feat[15] = snr_ema;

            /* --- 6) TAIへ送るペイロード作成 --- */
            msg_ai_req_t ai;
            ai.tim = mir->tim;
            /* 可視化には平滑済みスペクトル p128 を使用 */
            for (int j=0;j<BIN128;j++) ai.spectrum[j] = p128[j];
            for (int f=0; f<FEAT_DIM; f++) ai.feat[f] = feat[f];



            //send_ai_req(TRUE, (msg_ai_req_t *)mir);
            send_ai_req(TRUE, &ai);
            send_led_req(TRUE, TLED_BLUE, TLED_PAT_BLINK_FAST, 3);

        }
        else if (pum->msgid == MSGID_TAI_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
            msg_ai_res_t *pmar = (msg_ai_res_t *)&pum->pyload;
            send_net_req(pum->result, pmar->tim, pmar->spectrum, IMU_REC_MAX /2);
        }
        else if (pum->msgid == MSGID_TNET_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
            send_led_req(TRUE, TLED_RED, TLED_PAT_BLINK_SLOW, 1);
        }
        else if (pum->msgid == MSGID_TLED_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
        }

        if (er == E_OK) {
            er = tk_rel_mpf(pum->mpfid, pum);
            if (er != E_OK) {
                APP_ERR_PRINT("error rel_mpf:%d\n", er);
                continue;
            }
        }

    }

}

/**
 * MSGID_TAI_REQを送信
 *
 * AI要求をTAIへ送信
 * @param[in] result 結果
 * @param[in] data 送信データへのポインタ
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_ai_req(INT result, msg_ai_req_t *data)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_ai_req_t *mar = NULL;

    er = tk_get_mpf(MPFID_LARGE, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TAI_REQ;
    pum->srctsk = TSKID_TAPP;
    pum->dsttsk = TSKID_TAI;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_LARGE;

    mar = (msg_ai_req_t *)&pum->pyload;
    memcpy(mar, data, sizeof(msg_ai_req_t));

    er = tk_snd_mbx( MBXID_TAI, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


/**
 * MSGID_TNET_REQを送信
 *
 * ネットワーク送信要求をTNETへ送信
 * @param[in] result 結果
 * @param[in] data 送信データへのポインタ
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_net_req(INT result, SYSTIM tim, const float *spectrum, size_t spectrum_len)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_net_req_t *pmnr = NULL;

    if (!spectrum || spectrum_len > (IMU_REC_MAX /2)) return E_PAR;

    // 固定長メモリを取得
    er = tk_get_mpf(MPFID_MEDIUM, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    // メッセージ構造体を作成
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TNET_REQ;
    pum->srctsk = TSKID_TAPP;
    pum->dsttsk = TSKID_TNET;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_MEDIUM;

    pmnr = (msg_net_req_t *)&pum->pyload;
    pmnr->tim = tim;
    memcpy(pmnr->spectrum, spectrum, spectrum_len * sizeof(float32_t));

    // メッセージ送信
    er = tk_snd_mbx( MBXID_TNET, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


/**
 * MSGID_TLED_REQを送信
 *
 * LED制御要求をTLEDへ送信
 * @param[in] led LED種
 * @param[in] pattern 点灯パターン
 * @param[in] blink_count 点滅回数
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_led_req(INT result, UB led, UB pattern, W blink_count)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_led_req_t *mlr = NULL;

    // 固定長メモリを取得
    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    // メッセージ構造体を作成
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TLED_REQ;
    pum->srctsk = TSKID_TAPP;
    pum->dsttsk = TSKID_TLED;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_SMALL;

    mlr = (msg_led_req_t *)&pum->pyload;
    mlr->led = led;
    mlr->pattern = pattern;
    mlr->blink_count = blink_count;

    // メッセージ送信
    er = tk_snd_mbx( MBXID_TLED, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}

