/*
 * PPPAdapter.cpp
 *
 *  Created on: 2019年9月18日
 *      Author: Gene Kong
 */

#include "PPPAdapter.h"

#ifdef  TAG
    #undef  TAG
#endif
#define TAG                             "PPPAdapter"

#define GSM_DEBUG       1

namespace FEmbed {

PPPAdapter::PPPAdapter(shared_ptr<PPPAdapterCallback> cb)
    : m_ppp_lock(new FEmbed::OSMutex()),
      m_cb(cb)
{
    m_state = PPPState_Uninit;
    m_ppp_pcb = NULL;
}

PPPAdapter::~PPPAdapter()
{
    // TODO Auto-generated destructor stub
}

// PPP status callback
//--------------------------------------------------------------
static void ppp_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    struct netif *pppif = ppp_netif(pcb);
    PPPAdapter *prot = (PPPAdapter *)(ctx);

    switch(err_code) {
        case PPPERR_NONE: {
            #if GSM_DEBUG
            elog_i(TAG, "status_cb: Connected");
            #if PPP_IPV4_SUPPORT
            elog_i(TAG,"   ipaddr    = %s", ipaddr_ntoa(&prot->netIf()->ip_addr));
            elog_i(TAG,"   gateway   = %s", ipaddr_ntoa(&prot->netIf()->gw));
            elog_i(TAG,"   netmask   = %s", ipaddr_ntoa(&prot->netIf()->netmask));
            #endif

            #if PPP_IPV6_SUPPORT
            elog_i(TAG,"   ip6addr   = %s", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
            #endif
            #endif
            prot->updatePPPState(PPPAdapter::PPPState_Connected);
            break;
        }
        case PPPERR_PARAM: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Invalid parameter");
            #endif
            break;
        }
        case PPPERR_OPEN: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Unable to open PPP session");
            #endif
            break;
        }
        case PPPERR_DEVICE: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Invalid I/O device for PPP");
            #endif
            break;
        }
        case PPPERR_ALLOC: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Unable to allocate resources");
            #endif
            break;
        }
        case PPPERR_USER: {
            /* ppp_free(); -- can be called here */
            #if GSM_DEBUG
            elog_w(TAG,"status_cb: User interrupt (disconnected)");
            #endif
            prot->updatePPPState(PPPAdapter::PPPState_Disconnected);
            break;
        }
        case PPPERR_CONNECT: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Connection lost");
            #endif
            prot->updatePPPState(PPPAdapter::PPPState_Disconnected);
            break;
        }
        case PPPERR_AUTHFAIL: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Failed authentication challenge");
            #endif
            break;
        }
        case PPPERR_PROTOCOL: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Failed to meet protocol");
            #endif
            break;
        }
        case PPPERR_PEERDEAD: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Connection timeout");
            #endif
            break;
        }
        case PPPERR_IDLETIMEOUT: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Idle Timeout");
            #endif
            break;
        }
        case PPPERR_CONNECTTIME: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Max connect time reached");
            #endif
            break;
        }
        case PPPERR_LOOPBACK: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Loopback detected");
            #endif
            break;
        }
        default: {
            #if GSM_DEBUG
            elog_e(TAG,"status_cb: Unknown error code %d", err_code);
            #endif
            break;
        }
    }
}

// === Handle sending data to GSM modem ===
//------------------------------------------------------------------------------
static u32_t ppp_output_callback(ppp_pcb *pcb,
                                 u8_t *data, u32_t len, void *ctx)
{
    (void ) pcb;
    PPPAdapter *prot = (PPPAdapter *)(ctx);
    return prot->write(data, len);
}

//------------------------------------------------------------------------------
uint32_t PPPAdapter::write(uint8_t *data, uint32_t len)
{
    if(m_cb)
        return m_cb->pppWrite(data, len);
    return 0;
}

void PPPAdapter::update(uint8_t *data, uint32_t len)
{
    pppos_input_tcpip(m_ppp_pcb, (u8_t*)data, len);
}

/**
 * Initial PPP interface for network.
 * @return initial failed or not.
 */
bool PPPAdapter::pppInit()
{
    IP4_ADDR(&m_ppp_netif.ip_addr, 192, 168 , 4, 1);
    IP4_ADDR(&m_ppp_netif.gw, 192, 168 , 4, 1);
    IP4_ADDR(&m_ppp_netif.netmask, 255, 255 , 255, 0);

    m_ppp_pcb = pppapi_pppos_create(&m_ppp_netif,
                 ppp_output_callback,
                 ppp_status_cb,
                 this);

    return m_ppp_pcb?true:false;
}

bool PPPAdapter::pppNegotiation()
{
    if(m_ppp_pcb)
    {
        pppapi_set_default(m_ppp_pcb);
        ppp_set_auth(m_ppp_pcb, PPPAUTHTYPE_PAP, "XCheng", NULL);
    }
    return pppapi_connect(m_ppp_pcb, 0)?false:true;
}

} /* namespace FEmbed */