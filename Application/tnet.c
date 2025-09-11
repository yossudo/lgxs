/*
 * tnet.c
 *
 *  UDP送信タスク（TAPP→TNET_REQ 受信→UDP送信→TNET_RES 応答）
 *
 *  前提：
 *   - 低レベルEtherドライバに "hetha" というT-Kernelデバイスがある
 *   - UDPは片方向（応答無し）
 */

#include <tk/tkernel.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "common_utils.h"
#include "user_res.h"
#include "tnet.h"

/*==============================*/
/*  ここから UDP 送信ユーティリティ */
/*==============================*/

#define ETHER_DEV_NAME   "hetha"
#define ETHER_TYPE_IP    0x0800
#define IP_PROTO_UDP     17

/* 送信先/送信元のMAC/IP/PORTは必要に応じて調整してください */
#define SRC_MAC  {0x74, 0x90, 0x50, 0xB0, 0xDD, 0x2A}
#define DST_MAC  {0x00, 0xE0, 0x4C, 0x68, 0x03, 0x24}  /* 例：udpホスト(blitz) */
#define SRC_IP   {192, 168, 137, 5}
#define DST_IP   {192, 168, 137, 1}                    /* 例：udpホスト(blitz) */
#define SRC_PORT 12345
#define DST_PORT 12345

/* 送信CSVの設定（サイズ削減のため間引き等が必要なら調整） */
#define LGXS_FS_HZ       (100.0f)                 /* サンプリング周波数 */
#define LGXS_FFT_N       (1024)                   /* FFTポイント */
#define LGXS_BIN_HZ      (LGXS_FS_HZ / LGXS_FFT_N)
#define LGXS_DOWNSAMPLE  (1)                      /* 2,4,8… にするとCSV縮小 */
#define LGXS_FLOAT_FMT   "%.3f"                   /* 小数フォーマット */

/* SYSTIM → U64（μsなど）の変換。環境のマクロに合わせる。 */
static inline unsigned long long systim_to_u64(SYSTIM t)
{
#ifdef SYSTIM_TO_UD
    return (unsigned long long)SYSTIM_TO_UD(t);   /* 既存マクロがあるなら利用（例：μs） */
#else
    return (unsigned long long)(t);
#endif
}

#pragma pack(1)
typedef struct {
    uint8_t  dst_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype;
} ether_header_t;

typedef struct {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t  src_ip[4];
    uint8_t  dst_ip[4];
} ip_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;  /* 今回は0（未計算）で送る */
} udp_header_t;
#pragma pack()

static inline uint16_t htons(uint16_t x)
{
    return (uint16_t)((x << 8) | (x >> 8));
}

/* IPヘッダチェックサム計算 */
static uint16_t ip_checksum(uint16_t *buf, int len)
{
    uint32_t sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(uint8_t *)buf;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

/* 任意のpayloadをUDPで送る（生イーサ組み立て） */
static ER send_udp_packet(ID dd, const uint8_t *payload, uint16_t payload_len)
{
    /* イーサフレームの最大長を確保（十分に大きいワーク領域） */
    static uint8_t buffer[1500];

    uint8_t src_mac[6] = SRC_MAC;
    uint8_t dst_mac[6] = DST_MAC;
    uint8_t src_ip[4]  = SRC_IP;
    uint8_t dst_ip[4]  = DST_IP;

    ether_header_t *eth = (ether_header_t *)buffer;
    ip_header_t    *ip  = (ip_header_t *)(eth + 1);
    udp_header_t   *udp = (udp_header_t *)(ip + 1);
    uint8_t        *pld = (uint8_t *)(udp + 1);

    if ((sizeof(buffer) - (size_t)((uint8_t*)pld - buffer)) < payload_len) {
        return E_PAR;
    }

    /* ---- Ethernet ヘッダ ---- */
    memcpy(eth->dst_mac, dst_mac, 6);
    memcpy(eth->src_mac, src_mac, 6);
    eth->ethertype = htons(ETHER_TYPE_IP);

    /* ---- IP ヘッダ ---- */
    ip->ver_ihl  = 0x45;                 /* IPv4, IHL=5(20byte) */
    ip->tos      = 0;
    ip->tot_len  = htons((uint16_t)(sizeof(ip_header_t) + sizeof(udp_header_t) + payload_len));
    ip->id       = 0;
    ip->frag_off = 0;
    ip->ttl      = 64;
    ip->protocol = IP_PROTO_UDP;
    ip->checksum = 0;                    /* 一旦0で埋め、後で計算 */
    memcpy(ip->src_ip, src_ip, 4);
    memcpy(ip->dst_ip, dst_ip, 4);
    ip->checksum = ip_checksum((uint16_t *)ip, (int)sizeof(ip_header_t));

    /* ---- UDP ヘッダ ---- */
    udp->src_port = htons(SRC_PORT);
    udp->dst_port = htons(DST_PORT);
    udp->length   = htons((uint16_t)(sizeof(udp_header_t) + payload_len));
    udp->checksum = 0;                   /* 今回は未計算(0)で送る */

    /* ---- Payload ---- */
    memcpy(pld, payload, payload_len);

    /* ---- 送信 ---- */
    uint16_t total_len = (uint16_t)(sizeof(ether_header_t) + sizeof(ip_header_t)
                                    + sizeof(udp_header_t) + payload_len);

    SZ asize = 0;
    ER er = tk_swri_dev(dd, 0, (char *)buffer, total_len, &asize);
    return er;
}

/* CSVペイロード構築：ts/result/メタ情報/FFT配列 */
static int build_fft_csv(char *dst, size_t cap,
                         SYSTIM ts, const float *spec, size_t n, int result)
{
    if (!dst || !spec || cap < 32) return -1;

    size_t len = 0;
    const unsigned long long ts_us = systim_to_u64(ts);

    /* ヘッダ部（ts, result, n, bin_hz） */
    int w = snprintf(dst + len, cap - len,
                     "ts_us=%llu,result=%d,n=%u,bin_hz=%.6f,fft=",
                     ts_us, result, (unsigned)n, (double)LGXS_BIN_HZ);
    if (w < 0) return -2;
    len += (size_t)w;

    /* スペクトル本体（必要に応じて間引き） */
    for (size_t i = 0; i < n; i += LGXS_DOWNSAMPLE) {
        if (len + 16 >= cap) break;  /* 余裕なければ打ち切り */
        w = snprintf(dst + len, cap - len,
                     (i + LGXS_DOWNSAMPLE < n) ? LGXS_FLOAT_FMT "," : LGXS_FLOAT_FMT "\n",
                     (double)spec[i]);
        if (w < 0) return -3;
        len += (size_t)w;
    }
    return (int)len;
}

/*==============================*/
/*  TNET タスク実装              */
/*==============================*/

LOCAL ID s_eth_dd = E_PAR; /* ether デバイスのディスクリプタを保持 */

/* 応答メッセージ：TAPPへ MSGID_TNET_RES を返す */
LOCAL ER send_net_res(INT result)
{
    ER er;
    user_msg_t *pum = NULL;

    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("[TNET] get_mpf err:%d\n", er);
        return er;
    }

    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TNET_RES;
    pum->srctsk = TSKID_TNET;
    pum->dsttsk = TSKID_TAPP;
    pum->result = (UH) result;
    pum->mpfid  = MPFID_SMALL;

    er = tk_snd_mbx(MBXID_TAPP, (T_MSG *)pum);
    if (er != E_OK) {
        APP_ERR_PRINT("[TNET] snd_mbx err:%d\n", er);
        return er;
    }

    return E_OK;
}

/* 初期化：ether デバイス open */
LOCAL void init_task_tnet(void)
{
    if (s_eth_dd >= E_OK) return;

    s_eth_dd = tk_opn_dev((UB *)ETHER_DEV_NAME, TD_UPDATE);
    if (s_eth_dd < E_OK) {
        APP_ERR_PRINT("[TNET] tk_opn_dev('%s') failed: %d\n", ETHER_DEV_NAME, s_eth_dd);
    } else {
        APP_PRINT("[TNET] device '%s' opened (dd=%d)\n", ETHER_DEV_NAME, s_eth_dd);
    }
}

/* TNET メインタスク */
EXPORT void task_tnet(INT stacd, void *exinf)
{
    //(void)stacd; (void)exinf;

    APP_PRINT("[TNET started]\n");

    init_task_tnet();

    ER er;
    user_msg_t *pum = NULL;

    while (1) {
        er = tk_rcv_mbx(MBXID_TNET, (T_MSG **)&pum, TMO_FEVR);
        if (er != E_OK) {
            APP_ERR_PRINT("[TNET] rcv_mbx err:%d\n", er);
            continue;
        }

        if (pum->msgid == MSGID_TNET_REQ) {
            /* TAPP→TNET：AI判定結果は user_msg_t.result に格納されている前提 */
            INT ai_result = pum->result;

            if (s_eth_dd >= E_OK) {
                /* 固有ペイロードを取り出す（timestamp + spectrum[]） */
                msg_net_req_t *req = (msg_net_req_t *)&pum->pyload;

                /* CSV生成（ts/result/メタ情報/FFT） */
                char csv[1400];
                int len = build_fft_csv(csv, sizeof(csv),
                                        req->tim, req->spectrum, IMU_REC_MAX /2,
                                        (int)ai_result);
                if (len < 0) {
                    APP_ERR_PRINT("[TNET] build_fft_csv err:%d\n", len);
                    send_net_res(E_PAR);
                } else {
                    /* UDP送信（片方向・応答なし） */
                    ER snd = send_udp_packet(s_eth_dd, (const uint8_t *)csv, (uint16_t)len);
                    if (snd < E_OK) {
                        APP_ERR_PRINT("[TNET] UDP send error=%d\n", snd);
                    } else {
                        APP_PRINT("[TNET] UDP sent %dB (ts+FFT)\n", len);
                    }
                    send_net_res((snd < E_OK) ? E_IO : E_OK);
                }
            } else {
                APP_ERR_PRINT("[TNET] ether device not opened.\n");
                send_net_res(E_IO);
            }
        } else {
            APP_ERR_PRINT("[TNET] unexpected msgid=%d\n", pum->msgid);
        }

        /* メモリ返却 */
        er = tk_rel_mpf(pum->mpfid, pum);
        if (er != E_OK) {
            APP_ERR_PRINT("[TNET] rel_mpf err:%d\n", er);
        }
    }
}
