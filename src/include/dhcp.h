#ifndef DHCP_H
#define DHCP_H

#include <stddef.h>
#include <stdint.h>


#define DHCP_OP_REQUEST 1
#define DHCP_OP_REPLY 2

#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET 6

#define DHCP_MAGIC_COOKIE 0x63825363

#define DHCP_OPTION_PAD                 0
#define DHCP_OPTION_SUBNET_MASK         1
#define DHCP_OPTION_ROUTER              3
#define DHCP_OPTION_DNS_SERVER          6
#define DHCP_OPTION_REQ_IP_ADDR         50
#define DHCP_OPTION_LEASE_TIME          51
#define DHCP_OPTION_MSG_TYPE            53
#define DHCP_OPTION_SERVER_ID           54
#define DHCP_OPTION_END                 255

// DHCP Message Types (for option 53)
#define DHCP_MESSAGE_TYPE_DISCOVER      1
#define DHCP_MESSAGE_TYPE_OFFER         2
#define DHCP_MESSAGE_TYPE_REQUEST       3
#define DHCP_MESSAGE_TYPE_DECLINE       4
#define DHCP_MESSAGE_TYPE_ACK           5
#define DHCP_MESSAGE_TYPE_NAK           6
#define DHCP_MESSAGE_TYPE_RELEASE       7
#define DHCP_MESSAGE_TYPE_INFORM        8

typedef struct {
  uint8_t op;
  uint8_t htype;
  uint8_t hlen;
  uint8_t hops;
  uint32_t xid;
  uint16_t secs;
  uint16_t flags;
  uint32_t ciaddr;
  uint32_t yiaddr;
  uint32_t siaddr;
  uint32_t giaddr;
  uint8_t chaddr[16];
  char sname[64];
  char file[128];
  uint32_t magic_cookie;
  uint8_t options[312];
} __attribute__((packed)) dhcp_packet_t;

typedef enum {
    DHCP_STATE_INIT,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING
} dhcp_state_t;

void dhcp_discover();
void dhcp_init();

#endif
