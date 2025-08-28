/*
 *----------------------------------------------------------------------
 *    micro T-Kernel 3.0 BSP 2.0
 *
 *    Copyright (C) 2023-2024 by Ken Sakamura.
 *    This software is distributed under the T-License 2.1.
 *----------------------------------------------------------------------
 *
 *    Released by TRON Forum(http://www.tron.org) at 2024/03.
 *
 *----------------------------------------------------------------------
 */
#include <sys/machine.h>
#include <config_bsp/ra_fsp/config_bsp.h>

#ifdef MTKBSP_RAFSP
#if DEVCNF_USE_HAL_ETH

#include <stdlib.h>

#include <tk/tkernel.h>
#include <tk/device.h>

#include <sysdepend/ra_fsp/cpu_status.h>
#include <mtkernel/kernel/knlinc/tstdlib.h>
#include <mtkernel/device/common/drvif/msdrvif.h>
#include "hal_eth_cnf.h"
#include "r_ioport.h"

#if 1
// ETH_A_CH
#define ETH_RESET_IO ETH_A_RST
#else
// ETH_B_CH
#define ETH_RESET_IO ETH_B_RST_CAM_D10
#endif

/*Ether Device driver Control block*/
typedef struct {
    ether_ctrl_t   *heth;       // Ether handle
    ether_cfg_t    *ceth;       // Ether config
    ID              devid;      // Device ID
    UINT            omode;      // Open mode
    UW              unit;       // Unit no
    ID              evtmbfid;   // MBF ID for event notification
} T_HAL_ETH_DCB;

/* Interrupt detection flag */
LOCAL ID    id_flgid;
LOCAL T_CFLG    id_flg  = {
        .flgatr     = TA_TFIFO | TA_WMUL,
        .iflgptn    = 0,
};

#if TK_SUPPORT_MEMLIB
LOCAL T_HAL_ETH_DCB *dev_eth_cb[DEV_HAL_ETH_UNITNM];
#define     get_dcb_ptr(unit)   (dev_eth_cb[unit])
#else
LOCAL T_HAL_ETH dev_eth_cb[DEV_HAL_ETH_UNITNM];
#define     get_dcb_ptr(unit)   (&dev_eth_cb[unit])
#endif

LOCAL ER read_data(T_HAL_ETH_DCB *p_dcb, T_DEVREQ *req)
{
    UINT        wflgptn, rflgptn;
    fsp_err_t   fsp_err;
    ER      err;
    //UB *p_recv;
    //UW  recv_len;

    wflgptn = 1 << p_dcb->unit;
    tk_clr_flg(id_flgid, ~wflgptn);

    fsp_err = R_ETHER_Read(p_dcb->heth, req->buf, (UW*)&req->size);
    if(fsp_err != FSP_SUCCESS) return E_IO;

    err = tk_wai_flg(id_flgid, wflgptn, TWF_ANDW | TWF_BITCLR, &rflgptn, DEV_HAL_ETH_TMOUT);
    if(err >= E_OK) {
        if(err >= E_OK) req->asize = req->size;
    }

    return err;
}


LOCAL ER write_data(T_HAL_ETH_DCB *p_dcb, T_DEVREQ *req)
{
    UINT        wflgptn, rflgptn;
    fsp_err_t   fsp_err;
    ER      err;

    wflgptn = 1 << p_dcb->unit;
    tk_clr_flg(id_flgid, ~wflgptn);

    fsp_err = R_ETHER_Write(p_dcb->heth, (UB*)req->buf, req->size);
    if(fsp_err != FSP_SUCCESS) return E_IO;

    err = tk_wai_flg(id_flgid, wflgptn, TWF_ANDW | TWF_BITCLR, &rflgptn, DEV_HAL_ETH_TMOUT);
    if(err >= E_OK) {
        if(err >= E_OK) req->asize = req->size;
    }

    return err;
}


/*----------------------------------------------------------------------
 * mSDI I/F function
 */
/*
 * Open device
 */
LOCAL ER dev_eth_openfn(ID devid, UINT omode, T_MSDI *msdi)
{
    T_HAL_ETH_DCB   *p_dcb;
    fsp_err_t   fsp_err;

    // RESET_N 制御
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_RESET_IO, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    R_IOPORT_PinWrite(&g_ioport_ctrl, ETH_RESET_IO, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

    p_dcb = (T_HAL_ETH_DCB*)(msdi->dmsdi.exinf);
    if(p_dcb->heth == NULL) return E_IO;

    p_dcb->omode = omode;

    p_dcb->ceth->p_context = p_dcb;
    fsp_err =  R_ETHER_Open(p_dcb->heth, p_dcb->ceth);

    // リンクアップを待つ
    while (1)
    {
        fsp_err = g_ether0.p_api->linkProcess(p_dcb->heth);
        if (FSP_SUCCESS == fsp_err) break;

        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }

    return (fsp_err == FSP_SUCCESS)?E_OK:E_IO;
}


/* Read (Receive) */
LOCAL ER dev_eth_readfn(T_DEVREQ *req, T_MSDI *p_msdi)
{
    T_HAL_ETH_DCB   *p_dcb;
    ER      err;

    UINT unit = (UINT)(req->exinf);
    if (unit >= DEV_HAL_ETH_UNITNM) return E_PAR;

    p_dcb = (T_HAL_ETH_DCB*)(p_msdi->dmsdi.exinf);

    err = read_data(p_dcb, req);

    return err;
}

/* Write (Transmit) */
LOCAL ER dev_eth_writefn(T_DEVREQ *req, T_MSDI *p_msdi) {
    T_HAL_ETH_DCB   *p_dcb;
    ER      err;

    UINT unit = (UINT)(req->exinf);
    if (unit >= DEV_HAL_ETH_UNITNM) return E_PAR;

    p_dcb = (T_HAL_ETH_DCB*)(p_msdi->dmsdi.exinf);

    err = write_data(p_dcb, req);

    return err;
}

/*----------------------------------------------------------------------
 * Device driver initialization and registration
 */
EXPORT ER dev_init_hal_eth( UW unit, ether_instance_ctrl_t *heth, const ether_cfg_t *ceth)
{
    static ether_cfg_t    ceth_nc;

    T_HAL_ETH_DCB   *p_dcb;
    T_IDEV      idev;
    T_MSDI      *p_msdi;
    T_DMSDI     dmsdi;
    ER      err;
    INT     i;

    if( unit >= DEV_HAL_ETH_UNITNM) return E_PAR;

#if TK_SUPPORT_MEMLIB
    p_dcb = (T_HAL_ETH_DCB*)Kmalloc(sizeof(T_HAL_ETH_DCB));
    if( p_dcb == NULL) return E_NOMEM;
    dev_eth_cb[unit]    = p_dcb;
#else
    p_dcb = &dev_eth_cb[unit];
#endif

    id_flgid = tk_cre_flg(&id_flg);
    if(id_flgid <= E_OK) {
        err = (ER)id_flgid;
        goto err_1;
    }

    /* Device registration information */
    dmsdi.exinf = p_dcb;
    dmsdi.drvatr    = 0;            /* Driver attributes */
    dmsdi.devatr    = TDK_UNDEF;        /* Device attributes */
    dmsdi.nsub  = 0;            /* Number of sub units */
    dmsdi.blksz = 1;            /* Unique data block size (-1 = unknown) */
    dmsdi.openfn    = dev_eth_openfn;
    dmsdi.closefn   = NULL;
    dmsdi.readfn    = dev_eth_readfn;
    dmsdi.writefn   = dev_eth_writefn;

    knl_strcpy( (char*)dmsdi.devnm, DEVNAME_HAL_ETH);
    i = knl_strlen(DEVNAME_HAL_ETH);
    dmsdi.devnm[i] = (UB)('a' + unit);
    dmsdi.devnm[i+1] = 0;

    err = msdi_def_dev( &dmsdi, &idev, &p_msdi);
    if(err != E_OK) goto err_1;

    ceth_nc = *ceth;

    p_dcb->heth = heth;
    p_dcb->ceth = &ceth_nc;
    p_dcb->devid = p_msdi->devid;
    p_dcb->unit = unit;
    p_dcb->evtmbfid = idev.evtmbfid;

    return E_OK;

err_1:
#if TK_SUPPORT_MEMLIB
    Kfree(p_dcb);
#endif
    return err;
}




#endif
#endif
