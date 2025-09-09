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

/* T-Kernel デバイスディスクリプタを受け取り、AI結果をペイロードに載せてUDP送信 */
static void send_udp_packet(ID dd, INT ai_result)
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
    uint8_t        *payload = (uint8_t *)(udp + 1);

    /* ---- ペイロード生成（AI結果を文字列で送る） ---- */
    /* 例: "LGXS AI=1\n" */
    char msg[64];
    int msg_len = snprintf(msg, sizeof(msg), "LGXS AI=%d\n", (int)ai_result);
    if (msg_len < 0) msg_len = 0;
    if (msg_len > (int)sizeof(msg)) msg_len = (int)sizeof(msg);
    memcpy(payload, msg, (size_t)msg_len);

    /* ---- Ethernet ヘッダ ---- */
    memcpy(eth->dst_mac, dst_mac, 6);
    memcpy(eth->src_mac, src_mac, 6);
    eth->ethertype = htons(ETHER_TYPE_IP);

    /* ---- IP ヘッダ ---- */
    ip->ver_ihl  = 0x45;                 /* IPv4, IHL=5(20byte) */
    ip->tos      = 0;
    ip->tot_len  = htons((uint16_t)(sizeof(ip_header_t) + sizeof(udp_header_t) + msg_len));
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
    udp->length   = htons((uint16_t)(sizeof(udp_header_t) + msg_len));
    udp->checksum = 0;                   /* 今回は未計算(0)で送る */

    /* ---- 送信 ---- */
    uint16_t total_len = (uint16_t)(sizeof(ether_header_t) + sizeof(ip_header_t)
                                    + sizeof(udp_header_t) + msg_len);

    SZ asize = 0;
    tk_swri_dev(dd, 0, (char *)buffer, total_len, &asize);

    return;
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
    (void)stacd; (void)exinf;

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
            /* TAPP→TNET：AI判定結果は result に格納されている前提 */
            INT ai_result = pum->result;

            if (s_eth_dd >= E_OK) {
                send_udp_packet(s_eth_dd, ai_result);  /* 片方向送信（応答は待たない） */
                send_net_res(ai_result);               /* 送信したことをTAPPへ応答 */
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
