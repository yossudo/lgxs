/*
 * app_main.c
 *
 *  Created on: 2025/06/15
 *      Author: yoshi
 */


#include <tk/tkernel.h>
#include "common_utils.h"
#include "user_res.h"
#include "hal_data.h"
#include <string.h>
#include "tmpu_tsk.h.old"  // ← 追加

#if 0

#define ETHER_EXAMPLE_MAXIMUM_ETHERNET_FRAME_SIZE     (1514)
#define ETHER_EXAMPLE_TRANSMIT_ETHERNET_FRAME_SIZE    (60)
#define ETHER_EXAMPLE_SOURCE_MAC_ADDRESS              0x74, 0x90, 0x50, 0x00, 0x79, 0x01
#define ETHER_EXAMPLE_DESTINATION_MAC_ADDRESS         0x74, 0x90, 0x50, 0x00, 0x79, 0x02
#define ETHER_EXAMPLE_FRAME_TYPE                      0x00, 0x2E
#define ETHER_EXAMPLE_PAYLOAD                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                   \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                   \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                   \
/* Receive data buffer */
static uint8_t gp_read_buffer[ETHER_EXAMPLE_MAXIMUM_ETHERNET_FRAME_SIZE] = {0};
/* Transmit data buffer */
static uint8_t gp_send_data[ETHER_EXAMPLE_TRANSMIT_ETHERNET_FRAME_SIZE] =
{
    ETHER_EXAMPLE_DESTINATION_MAC_ADDRESS, /* Destination MAC address */
    ETHER_EXAMPLE_SOURCE_MAC_ADDRESS,      /* Source MAC address */
    ETHER_EXAMPLE_FRAME_TYPE,              /* Type field */
    ETHER_EXAMPLE_PAYLOAD                  /* Payload value (46byte) */
};

void ether_basic_example (void)
{
    extern const ioport_instance_t g_ioport;  // FSPの自動生成コードより
    g_ioport.p_api->open(g_ioport.p_ctrl, g_ioport.p_cfg);

    // RESET_N を制御（例: P706）
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ



    fsp_err_t err = FSP_SUCCESS;
    /* Source MAC Address */
    static uint8_t mac_address_source[6] = {ETHER_EXAMPLE_SOURCE_MAC_ADDRESS};
    uint32_t read_data_size = 0;
    //g_ether0_cfg.p_mac_address = mac_address_source;
    /* Open the ether instance with initial configuration. */
    err = R_ETHER_Open(&g_ether0_ctrl, &g_ether0_cfg);
    /* Handle any errors. This function should be defined by the user. */
    assert(FSP_SUCCESS == err);
    do
    {
        /* When the Ethernet link status read from the PHY-LSI Basic Status register is link-up,
         * Initializes the module and make auto negotiation. */
        err = R_ETHER_LinkProcess(&g_ether0_ctrl);
    } while (FSP_SUCCESS != err);
    /* Transmission is non-blocking. */
    /* User data copy to internal buffer and is transferred by DMA in the background. */
    err = R_ETHER_Write(&g_ether0_ctrl, (void *) gp_send_data, sizeof(gp_send_data));
    assert(FSP_SUCCESS == err);
    /* received data copy to user buffer from internal buffer. */
    err = R_ETHER_Read(&g_ether0_ctrl, (void *) gp_read_buffer, &read_data_size);
    assert(FSP_SUCCESS == err);
    /* Disable transmission and receive function and close the ether instance. */
    R_ETHER_Close(&g_ether0_ctrl);
}

#endif


#if 0
#define ETHER_TX_BUFFER_SIZE  1500
#define ETHER_RX_BUFFER_SIZE  1500

//static uint8_t tx_buffer[ETHER_TX_BUFFER_SIZE] = "Hello, PC!\r\n";
static uint8_t rx_buffer[ETHER_RX_BUFFER_SIZE];

// 初期化処理（g_ether0をオープン）
fsp_err_t ether_init(void)
{
    // RESET_N を制御（例: P706）
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ

    fsp_err_t err = g_ether0.p_api->open(g_ether0.p_ctrl, g_ether0.p_cfg);
    if (err != FSP_SUCCESS) return err;

#if 1
    uint32_t phy_status = 0;
    err = R_ETHER_PHY_Read(g_ether_phy0.p_ctrl, 0x01, &phy_status);  // 0x01: BASIC STATUS REGISTER
    if (err == FSP_SUCCESS)
    {
        bool link_up = (phy_status & 0x0004); // Bit2: Link Status
        APP_PRINT("Link Status: %s\n", link_up ? "UP" : "DOWN");
    }
#endif

    APP_PRINT("Ethernet link is up!\r\n");
    return FSP_SUCCESS;
}

fsp_err_t ether_send(void)
{
    uint8_t ether_frame[64]; // 最小サイズに合わせる

    // 宛先 MAC：ブロードキャスト
    uint8_t dst_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    // 送信元 MAC：RA8D1のMAC
    uint8_t src_mac[6] = {0x74, 0x90, 0x50, 0xb0, 0xdd, 0x2a};
    //R_ETHER_ReadMacAddress(g_ether0.p_ctrl, src_mac);

    // 宛先MACコピー
    memcpy(&ether_frame[0], dst_mac, 6);
    // 送信元MACコピー
    memcpy(&ether_frame[6], src_mac, 6);

    // EtherType：0x0800（IPv4）※とりあえずダミー
    ether_frame[12] = 0x08;
    ether_frame[13] = 0x00;

    // ペイロードデータ："Hello, PC!"
    const char *payload = "Hello, PC!";
    memcpy(&ether_frame[14], payload, strlen(payload));

    // 必要に応じて64バイトにパディング
    size_t frame_len = 14 + strlen(payload);
    if (frame_len < 60) {
        memset(&ether_frame[frame_len], 0, 60 - frame_len);
        frame_len = 60;
    }

    // 送信（CRCはFSPドライバが付けてくれる）
    return g_ether0.p_api->write(g_ether0.p_ctrl, ether_frame, frame_len);
}

// 受信処理
fsp_err_t ether_receive(void)
{
    uint32_t length = 0;
    fsp_err_t err = g_ether0.p_api->read(g_ether0.p_ctrl, rx_buffer, &length);

    if (FSP_SUCCESS == err && length > 0)
    {
        // 必要に応じて内容を処理
        rx_buffer[length] = '\0';  // NULL終端
        APP_PRINT("Received: %s\n", rx_buffer);
    }

    return err;
}


void ether_task(void)
{
    if (FSP_SUCCESS != ether_init()) {
        APP_ERR_PRINT("[app_main] ether_init failed: %d\n");
        while (1); // エラー処理
    }

    while (1)
    {
        ether_send();        // 送信
        ether_receive();     // 受信
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
#endif

#if 1
void ether_callback(ether_callback_args_t * p_args)
{
    /* 必要に応じてイベント処理 */
}
#endif


#if 0
#include "hal_data.h"
#include "r_ether.h"
#include "r_ether_api.h"
#include "r_ether_phy.h"  // phy用インタフェース

#define TEST_BUFFER_SIZE 64
#define MAC_ADDR { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 }

//static ether_ctrl_t g_ether_ctrl;
static uint8_t g_tx_buffer[TEST_BUFFER_SIZE] = "Hello from Ether!";
static uint8_t g_rx_buffer[TEST_BUFFER_SIZE];
static uint32_t g_rx_len = 0;
static uint8_t g_mac_address[6] = MAC_ADDR;

// NOTE: 実際にはFSPの設定でpp_ether_buffersも初期化が必要です。
// 以下は最小構成なのでゼロで妥協
static uint8_t * g_ether_buffers[2] = { NULL, NULL };

static ether_cfg_t g_ether_cfg =
{
    .channel             = 1,  // ETHB (RA8D1でEthernet Channel Bを使用)
    .zerocopy            = ETHER_ZEROCOPY_DISABLE,
    .multicast           = ETHER_MULTICAST_DISABLE,
    .promiscuous         = ETHER_PROMISCUOUS_DISABLE,
    .flow_control        = ETHER_FLOW_CONTROL_DISABLE,
    .padding             = ETHER_PADDING_3BYTE,
    .padding_offset      = 0,
    .broadcast_filter    = 0,
    .p_mac_address       = g_mac_address,
    .num_tx_descriptors  = 1,
    .num_rx_descriptors  = 1,
    .pp_ether_buffers    = g_ether_buffers,
    .ether_buffer_size   = TEST_BUFFER_SIZE,
    .irq                 = EDMAC0_EINT_IRQn,
    .interrupt_priority  = 3,
    .p_callback          = NULL,
    .p_ether_phy_instance = &g_ether_phy0,  // FSPでETH PHYが有効である必要あり
    .p_context           = NULL,
    .p_extend            = NULL
};

static ether_instance_t g_ether_instance =
{
    .p_ctrl = &g_ether0_ctrl,
    .p_cfg  = &g_ether0_cfg,
    .p_api  = &g_ether_on_ether  // この記述は r_ether.c に定義あり
};

void ether_direct_test(void)
{
    fsp_err_t err;

    // RESET_N を制御（例: P706）
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ
    // ドライバ初期化
    err = g_ether_instance.p_api->open(g_ether_instance.p_ctrl, g_ether_instance.p_cfg);
    if (FSP_SUCCESS != err) { __BKPT(); return; }

    // リンクアップを待つ
    while (1)
    {
        err = g_ether_instance.p_api->linkProcess(g_ether_instance.p_ctrl);
        if (FSP_SUCCESS == err) break;

        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }

    // 書き込みテスト
    err = g_ether_instance.p_api->write(g_ether_instance.p_ctrl, g_tx_buffer, strlen((char *)g_tx_buffer));
    if (FSP_SUCCESS != err) { __BKPT(); return; }

    // 読み取りテスト
    err = g_ether_instance.p_api->read(g_ether_instance.p_ctrl, g_rx_buffer, &g_rx_len);
    if (FSP_SUCCESS == err && g_rx_len > 0)
    {
        g_rx_buffer[g_rx_len] = 0;  // Null terminate
    }

    // ドライバ終了
    g_ether_instance.p_api->close(g_ether_instance.p_ctrl);
}


#endif

#if 0
#include "r_ether.h"
#include "r_ether_api.h"
#include <stdio.h>
#include <string.h>

#define ETHER_FRAME_MIN_SIZE 60
#define ETHER_BUFFER_SIZE    1536
//#define ETHER_TYPE_CUSTOM_HI 0x88
//#define ETHER_TYPE_CUSTOM_LO 0xB5
#define ETHER_TYPE_CUSTOM_HI 0x08
#define ETHER_TYPE_CUSTOM_LO 0x00

static uint8_t rx_buffer[ETHER_BUFFER_SIZE] = {0};
static ether_instance_t g_ether_instance =
{
    .p_ctrl = &g_ether0_ctrl,
    .p_cfg  = &g_ether0_cfg,
    .p_api  = &g_ether_on_ether  // この記述は r_ether.c に定義あり
};

void ether_send_and_receive_test(void)
{
    fsp_err_t err;

    // RESET_N を制御（例: P706）
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_B_RST_CAM_D10, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS); // 10ms以上待つ
    // ドライバ初期化
    err = g_ether_instance.p_api->open(g_ether_instance.p_ctrl, g_ether_instance.p_cfg);
    if (FSP_SUCCESS != err) { __BKPT(); return; }

    // リンクアップを待つ
    while (1)
    {
        err = g_ether_instance.p_api->linkProcess(g_ether_instance.p_ctrl);
        if (FSP_SUCCESS == err) break;

        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }




    uint8_t tx_buffer[ETHER_FRAME_MIN_SIZE] = {0};
    const uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ブロードキャスト

    // 送信元MACアドレスを取得（FSP設定より）
    const uint8_t *src_mac = g_ether0_cfg.p_mac_address;

    // Ethernetヘッダ構築
    memcpy(&tx_buffer[0], dest_mac, 6);         // 宛先MAC
    memcpy(&tx_buffer[6], src_mac, 6);          // 送信元MAC
    tx_buffer[12] = ETHER_TYPE_CUSTOM_HI;       // EtherType（上位）
    tx_buffer[13] = ETHER_TYPE_CUSTOM_LO;       // EtherType（下位）

    // ペイロード
    const char *payload = "TRON POWER!";
    size_t payload_len = strlen(payload);
    memcpy(&tx_buffer[14], payload, payload_len);

    // フレーム長（最小60バイト）
    uint32_t send_len = (14 + payload_len < ETHER_FRAME_MIN_SIZE)
                        ? ETHER_FRAME_MIN_SIZE
                        : 14 + payload_len;

    // 送信
    err = R_ETHER_Write(&g_ether0_ctrl, tx_buffer, send_len);
    if (err != FSP_SUCCESS)
    {
        APP_ERR_PRINT("send error: %d\n", err);
        return;
    }
    else
    {
        APP_PRINT("send success (%ubyte): \"%s\"\n", send_len, payload);
    }

    // リンクプロセス（ポーリング）
    R_ETHER_LinkProcess(&g_ether0_ctrl);

    // 受信（受信バッファに何か来ていれば取得）
    uint32_t rx_len = 0;
    err = R_ETHER_Read(&g_ether0_ctrl, rx_buffer, &rx_len);
    if (err == FSP_SUCCESS && rx_len > 14)
    {
        // EtherType チェック
        if (rx_buffer[12] == ETHER_TYPE_CUSTOM_HI && rx_buffer[13] == ETHER_TYPE_CUSTOM_LO)
        {
            // 送信元MAC表示
            APP_PRINT("receive frame: %u byte\n", rx_len);
            APP_PRINT("source MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   rx_buffer[6], rx_buffer[7], rx_buffer[8],
                   rx_buffer[9], rx_buffer[10], rx_buffer[11]);

            // ペイロード表示（14バイト目以降）
            APP_PRINT("payload: ");
            for (uint32_t i = 14; i < rx_len; i++)
            {
                APP_PRINT("%c", rx_buffer[i]);
            }
            APP_PRINT("\n");
        }
        else
        {
            APP_ERR_PRINT("Other EtherType (0x%02X%02X) received\n", rx_buffer[12], rx_buffer[13]);
        }
    }
    else
    {
        APP_ERR_PRINT("no data received: err=%d, len=%u\n", err, rx_len);
    }
}

#endif


#if 1
#include <string.h>
#include <stdint.h>
#include "r_ether_api.h"
#include "r_ether.h"

#define UDP_PAYLOAD "Hello, UDP!"
#define UDP_PAYLOAD_LEN (sizeof(UDP_PAYLOAD) - 1)

#define SRC_MAC  {0x74, 0x90, 0x50, 0xB0, 0xDD, 0x2A}
//#define DST_MAC  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}  // Broadcast
#define DST_MAC  {0x00, 0xE0, 0x4C, 0x68, 0x03, 0x24}  // udp host (blitz)
#define SRC_IP   {192, 168, 137, 5}
//#define DST_IP   {192, 168, 137, 255}  // Broadcast within subnet
#define DST_IP   {192, 168, 137, 1}  // udp host (blitz)
#define SRC_PORT 12345
#define DST_PORT 12345

#define ETHER_TYPE_IP 0x0800
#define IP_PROTO_UDP  17

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
    uint16_t checksum;
} udp_header_t;
#pragma pack()

uint16_t calc_checksum(uint16_t *buf, int len)
{
    uint32_t sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1)
        sum += *(uint8_t *)buf;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

static inline uint16_t htons(uint16_t x)
{
    return (uint16_t)((x << 8) | (x >> 8));
}

void send_udp_packet(ID dd)
{
    static uint8_t buffer[1500];
    uint8_t *payload;
    uint8_t src_mac[6] = SRC_MAC;
    uint8_t dst_mac[6] = DST_MAC;
    uint8_t src_ip[4]  = SRC_IP;
    uint8_t dst_ip[4]  = DST_IP;

    ether_header_t *eth = (ether_header_t *)buffer;
    ip_header_t *ip = (ip_header_t *)(eth + 1);
    udp_header_t *udp = (udp_header_t *)(ip + 1);
    payload = (uint8_t *)(udp + 1);

    // Ethernet header
    memcpy(eth->dst_mac, dst_mac, 6);
    memcpy(eth->src_mac, src_mac, 6);
    eth->ethertype = htons(ETHER_TYPE_IP);

    // IP header
    ip->ver_ihl = 0x45;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(ip_header_t) + sizeof(udp_header_t) + UDP_PAYLOAD_LEN);
    ip->id = 0;
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IP_PROTO_UDP;
    ip->checksum = 0;
    memcpy(ip->src_ip, src_ip, 4);
    memcpy(ip->dst_ip, dst_ip, 4);
    ip->checksum = calc_checksum((uint16_t *)ip, sizeof(ip_header_t));

    // UDP header
    udp->src_port = htons(SRC_PORT);
    udp->dst_port = htons(DST_PORT);
    udp->length = htons(sizeof(udp_header_t) + UDP_PAYLOAD_LEN);
    udp->checksum = 0; // Optional

    // Payload
    memcpy(payload, UDP_PAYLOAD, UDP_PAYLOAD_LEN);

    uint16_t total_len = sizeof(ether_header_t) + sizeof(ip_header_t) + sizeof(udp_header_t) + UDP_PAYLOAD_LEN;
    //R_ETHER_Write(&g_ether0_ctrl, buffer, total_len);

    SZ asize = 0;
    ER ercd = tk_swri_dev(dd, 0, (char *)buffer, total_len, &asize);
    if (ercd < E_OK) {
        APP_ERR_PRINT("ERROR: tk_wri_dev failed\n");
    } else {
        APP_PRINT("Sent %d bytes\n", ercd);
    }

}

#endif

#if 1
#include "sysdepend/ra_fsp/device/device.h"    // デバイスドライバI/F
#include <string.h>

#define ETHER_DEV_NAME "hetha"
#define BUF_SIZE       1500        // Ethernetフレーム最大サイズ


#endif

#if 1
static bsp_leds_t s_leds;
void gpt_handler(UINT intno);
#endif


// TMPUタスク設定
//#define TMPU_TASK_PRIORITY  5  // 必要に応じて調整
//#define TMPU_TASK_STACK_SIZE  4096

//LOCAL ID id_tsk_tmpu;

// usermain() にタスク生成処理を追加
EXPORT INT usermain(void)
{
    APP_PRINT("[app_main] usermain started\n");

#if 1
    create_tasks();
    create_mailboxes();
    create_mem_pools();

    tk_slp_tsk(TMO_FEVR);
#endif

#if 0

    // TMPUタスク生成
    ctsk.tskatr = TA_HLNG | TA_RNG0;
    ctsk.stksz  = TMPU_TASK_STACK_SIZE;
    ctsk.task   = tmpu_main;
    ctsk.itskpri = TMPU_TASK_PRIORITY;
    ctsk.exinf  = NULL;

    id_tsk_tmpu = tk_cre_tsk(&ctsk);
    if (id_tsk_tmpu > 0) {
        tk_sta_tsk(id_tsk_tmpu, 0);
        APP_PRINT("[app_main] TMPU task started (ID=%d)\n", id_tsk_tmpu);
    } else {
        APP_ERR_PRINT("[app_main] TMPU task creation failed: %d\n", id_tsk_tmpu);
    }
#endif

#if 0
    APP_PRINT("=== Ether App Test Start ===\n");

#if 0
    ID  dd;

    // 1. ether デバイスを open
    dd = tk_opn_dev((UB *)ETHER_DEV_NAME, TD_UPDATE);
    if (dd < E_OK) {
        APP_PRINT("ERROR: tk_opn_dev failed\n");
        return -1;
    }
    tk_dly_tsk(1000);
    send_udp_packet(dd);
#endif

#if 0
    T_DINT dint = { .intatr = TA_HLNG, .inthdr = (FP)gpt_handler };
    tk_def_int(GPT_INTNO, &dint);

    R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);
    R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED1_BLUE,   BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED2_GREEN, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinWrite(&g_ioport_ctrl, USER_LED3_RED,  BSP_IO_LEVEL_LOW);

    fsp_err_t err;
    UW pclkd_freq_hz,period_counts;
    /* Get the source clock frequency (in Hz) */
    pclkd_freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD);
    pclkd_freq_hz >>= (uint32_t)(g_timer0.p_cfg->source_div);

#define TIMER_UNITS_MILLISECONDS  (1000U)        /* timer unit in millisecond */
#define CLOCK_TYPE_SPECIFIER      (1ULL)         /* type specifier */

    /* Convert period to PCLK counts so it can be set in hardware. */
    period_counts = (uint64_t)((g_timer0.p_cfg->period_counts * (pclkd_freq_hz * CLOCK_TYPE_SPECIFIER))  / TIMER_UNITS_MILLISECONDS);


    // GPTオープン＆開始
    err = R_GPT_Open (g_timer0.p_ctrl, g_timer0.p_cfg);
    if (FSP_SUCCESS != err) { /* ここでエラー処理 */ }

    err = R_GPT_Start(g_timer0.p_ctrl);
    if (FSP_SUCCESS != err) { /* ここでエラー処理 */ }


#endif
    tk_slp_tsk(TMO_FEVR);

#endif


#if 0
#define ETHER_FRAME_MIN_SIZE 60
#define ETHER_BUFFER_SIZE    1536

    ID  dd;
    ER  ercd;
    SZ size;
    SZ asize;
    static UB  tx_buf[ETHER_BUFFER_SIZE];
    //UB  rx_buf[BUF_SIZE];

    APP_PRINT("=== Ether App Test Start ===\n");

    // 1. ether デバイスを open
    dd = tk_opn_dev((UB *)ETHER_DEV_NAME, TD_UPDATE);
    if (dd < E_OK) {
        APP_PRINT("ERROR: tk_opn_dev failed\n");
        return -1;
    }

    // 2. 送信バッファにデータセット（例：UDPヘッダなど最低限の構成にしておく）
    // ※ここでは簡易にテスト用ダミーデータ
    // Ethernetヘッダ構築
    const uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // ブロードキャスト

    // 送信元MACアドレスを取得（FSP設定より）
    const uint8_t *src_mac = g_ether0_cfg.p_mac_address;

#define ETHER_TYPE_CUSTOM_HI 0x08
#define ETHER_TYPE_CUSTOM_LO 0x00

    memset(tx_buf, 0, sizeof(tx_buf));
    memcpy(&tx_buf[0], dest_mac, 6);         // 宛先MAC
    memcpy(&tx_buf[6], src_mac, 6);          // 送信元MAC
    tx_buf[12] = ETHER_TYPE_CUSTOM_HI;       // EtherType（上位）
    tx_buf[13] = ETHER_TYPE_CUSTOM_LO;       // EtherType（下位）

    // ペイロード
    const char *payload = "TRON POWER!";
    size_t payload_len = strlen(payload);
    memcpy(&tx_buf[14], payload, payload_len);

    // フレーム長（最小60バイト）
    uint32_t send_len = (14 + payload_len < ETHER_FRAME_MIN_SIZE)
                        ? ETHER_FRAME_MIN_SIZE
                        : 14 + payload_len;

    // 3. write で送信
    size = send_len;  // 最低フレームサイズを満たすように
    ercd = tk_swri_dev(dd, 0, (char *)tx_buf, size, &asize);
    if (ercd < E_OK) {
        APP_ERR_PRINT("ERROR: tk_wri_dev failed\n");
    } else {
        APP_PRINT("Sent %d bytes\n", ercd);
    }



    tk_slp_tsk(TMO_FEVR);

#endif

#if 0
    //ether_basic_example();

    // ether_task();

    //ether_direct_test();

    ether_send_and_receive_test();

    tk_slp_tsk(TMO_FEVR);

#endif

    return 0;
}

#if 0
static inline void led_toggle(ioport_port_pin_t pin) {
    bsp_io_level_t now;
    R_IOPORT_PinRead(&g_ioport_ctrl, pin, &now);
    R_IOPORT_PinWrite(&g_ioport_ctrl, pin,
        (now == BSP_IO_LEVEL_LOW) ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW);
}

void gpt_handler(UINT intno)
{
    static uint32_t tick = 0;

    volatile R_GPT0_Type * gpt = R_GPT0;   // ←使用チャンネルに合わせて
    uint32_t st = gpt->GTST;               // ステータス読取

    // 周期終端（オーバーフロー）だけ扱う
    if (st & R_GPT0_GTST_TCFPO_Msk) {

        if ((++tick % 100U) == 0U) {
            led_toggle(USER_LED3_RED);  // 赤LEDを点滅
        }

        // ---- ① GPT本体のフラグクリア ----
        // そのビットだけを0にする（他ビットは保持）
        gpt->GTST = st & ~R_GPT0_GTST_TCFPO_Msk;

    }

    // ---- ② ICU側のIRQ保留クリア ----
     R_BSP_IrqStatusClear(GPT_INTNO);

    // GPTの割込みフラグはFSPで適切に処理されるため、ここでの個別クリアは不要
    tk_ret_int();
}
#endif
