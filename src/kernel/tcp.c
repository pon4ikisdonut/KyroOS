#include "tcp.h"
#include "log.h"

tcp_tcb_t* active_tcbs = NULL;

void tcp_init() {
    klog(LOG_INFO, "TCP stack initialized (stub).");
}

void tcp_handle_packet(net_dev_t *net_dev, const ipv4_header_t *ip_hdr, const uint8_t *packet, size_t size) {
    (void)net_dev;
    (void)ip_hdr;
    (void)packet;
    (void)size;
    klog(LOG_INFO, "tcp_handle_packet (stub)");
}

tcp_tcb_t* tcp_create_tcb() {
    klog(LOG_INFO, "tcp_create_tcb (stub)");
    return NULL;
}

int tcp_connect_tcb(tcp_tcb_t* tcb) {
    (void)tcb;
    klog(LOG_INFO, "tcp_connect_tcb (stub)");
    return -1;
}

int tcp_send_tcb(tcp_tcb_t* tcb, const void* buf, size_t len, int flags) {
    (void)tcb;
    (void)buf;
    (void)len;
    (void)flags;
    klog(LOG_INFO, "tcp_send_tcb (stub)");
    return -1;
}

int tcp_recv_tcb(tcp_tcb_t* tcb, void* buf, size_t len, int flags) {
    (void)tcb;
    (void)buf;
    (void)len;
    (void)flags;
    klog(LOG_INFO, "tcp_recv_tcb (stub)");
    return -1;
}

int tcp_close_tcb(tcp_tcb_t* tcb) {
    (void)tcb;
    klog(LOG_INFO, "tcp_close_tcb (stub)");
    return -1;
}
