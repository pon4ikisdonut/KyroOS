#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h" // For ipv4_header_t, ip_send_packet
#include "net.h" // For net_dev_t
#include "thread.h" // For THREAD_BLOCKED, THREAD_READY, and thread_t

// TCP Header Flags
#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20
#define TCP_FLAG_ECE 0x40
#define TCP_FLAG_CWR 0x80

// TCP Header structure
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t acknowledgement_number;
    uint8_t  ns_reserved_dataoffset; // NS (1 bit), Reserved (3 bits), Data Offset (4 bits)
    uint8_t  flags;                  // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__((packed)) tcp_header_t;

// Minimal TCP Control Block (TCB)
typedef struct tcp_tcb {
    uint32_t local_ip;
    uint32_t remote_ip;
    uint16_t local_port;
    uint16_t remote_port;

    uint32_t snd_una; // Send Unacknowledged
    uint32_t snd_nxt; // Send Next
    uint32_t rcv_nxt; // Receive Next (expected sequence number)

    uint16_t snd_wnd; // Send window
    uint16_t rcv_wnd; // Receive window

    // State machine (simplified for connect-only)
    enum {
        TCP_STATE_CLOSED,
        TCP_STATE_LISTEN,
        TCP_STATE_SYN_SENT,
        TCP_STATE_SYN_RECEIVED,
        TCP_STATE_ESTABLISHED,
        TCP_STATE_FIN_WAIT_1,
        TCP_STATE_FIN_WAIT_2,
        TCP_STATE_CLOSE_WAIT,
        TCP_STATE_LAST_ACK,
        TCP_STATE_TIME_WAIT
    } state;

    // Receive buffer
    uint8_t* recv_buffer;
    size_t recv_buffer_size;
    volatile size_t recv_data_len;
    volatile size_t recv_read_idx;

    // For blocking calls
    thread_t* waiting_thread;
    
    // Timer for retransmissions (not implemented yet)
    uint64_t retransmit_timer;

    struct tcp_tcb* next; // For linked list of active TCBs
} tcp_tcb_t;

// Global list of active TCBs
extern tcp_tcb_t* active_tcbs;

// Functions for TCP management (kernel-side)
void tcp_init();
void tcp_handle_packet(net_dev_t *net_dev, const ipv4_header_t *ip_hdr, const uint8_t *packet, size_t size);
tcp_tcb_t* tcp_create_tcb();
int tcp_connect_tcb(tcp_tcb_t* tcb);
int tcp_send_tcb(tcp_tcb_t* tcb, const void* buf, size_t len, int flags);
int tcp_recv_tcb(tcp_tcb_t* tcb, void* buf, size_t len, int flags);
int tcp_close_tcb(tcp_tcb_t* tcb);

#endif // TCP_H
