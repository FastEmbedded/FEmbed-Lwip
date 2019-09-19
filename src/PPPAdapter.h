/*
 * PPPAdapter.h
 *
 *  Created on: 2019年9月18日
 *      Author: Gene Kong
 */

#ifndef LWIP_PPPPROTOCOL_H_
#define LWIP_PPPPROTOCOL_H_

#include "FEmbed.h"

#include "netif/ppp/pppapi.h"

namespace FEmbed {

class PPPAdapterCallback {
 public:
    virtual ~PPPAdapterCallback() {};
    virtual uint32_t pppWrite(uint8_t *data, uint32_t len)  = 0;
};

class PPPAdapter
{
 public:
    PPPAdapter(shared_ptr<PPPAdapterCallback> cb);
    virtual ~PPPAdapter();

    enum PPPState {
        PPPState_Disconnected,      ///< Disconnected from Internet
        PPPState_Connected,         ///< Connected to Internet
        PPPState_Idle,              ///< Disconnected from Internet, Task idle, waiting for reconnect request
        PPPState_Uninit,            ///< Task started, initializing PPPoS
    };

    void updatePPPState(PPPState state) {
        m_ppp_lock->lock();
        m_state = state;
        m_ppp_lock->unlock();
    }

    void lock() { m_ppp_lock->lock(); }
    void unlock() { m_ppp_lock->unlock(); }

    netif *netIf() { return &m_ppp_netif; }

    /**
     * When send data, please call write else update from read data.
     * @param data data need process!
     * @param len data length.
     * @return success write data len.
     */
    virtual uint32_t write(uint8_t *data, uint32_t len);
    virtual void update(uint8_t *data, uint32_t len);

    /**
     * Initial PPP interface for network.
     * @return initial failed or not.
     */
    bool pppInit();

    /**
     * LCP negotiation
     * AUTH negotiation
     * IPCP negotiation
     *
     * @return negotiation ok or not?
     */
    bool pppNegotiation();

 private:
    shared_ptr<FEmbed::OSMutex> m_ppp_lock;         ///< PPP lock for multi-thread operation.
    shared_ptr<PPPAdapterCallback> m_cb;
    PPPState m_state;
    ppp_pcb *m_ppp_pcb;
    struct netif m_ppp_netif;                      ///< Common PPP IP Interface

};

} /* namespace FEmbed */

#endif /* LWIP_PPPPROTOCOL_H_ */