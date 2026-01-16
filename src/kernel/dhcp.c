#include "dhcp.h"
#include "heap.h"
#include "ip.h"
#include "kstring.h"
#include "log.h"
#include "udp.h"
#include "thread.h" // For sleeping/yielding eventually



static dhcp_state_t current_dhcp_state = DHCP_STATE_INIT;
static uint32_t offered_ip_address = 0;
static uint32_t offered_subnet_mask = 0;
static uint32_t offered_router = 0;
static uint32_t offered_dns_server = 0;
static uint32_t dhcp_server_ip = 0;
static uint32_t lease_time = 0; // in seconds
static uint32_t renewal_time = 0; // T1, in seconds
static uint32_t rebind_time = 0; // T2, in seconds
// TODO: Integrate with a timer for lease management
// static uint32_t current_lease_start_time = 0; // Placeholder for timer integration


static uint32_t dhcp_xid = 0x12345678;

static uint8_t get_dhcp_option(const uint8_t* options, size_t options_len, uint8_t option_code, uint8_t* out_len, const uint8_t** out_value) {
    const uint8_t* ptr = options;
    while (ptr < options + options_len) {
        uint8_t code = *ptr++;
        if (code == DHCP_OPTION_PAD) {
            continue;
        }
        if (code == DHCP_OPTION_END) {
            break;
        }
        uint8_t len = *ptr++;
        if (ptr + len > options + options_len) {
            // Option extends beyond buffer
            return 0;
        }
        if (code == option_code) {
            if (out_len) *out_len = len;
            if (out_value) *out_value = ptr;
            return 1; // Found
        }
        ptr += len;
    }
    return 0; // Not found
}

void dhcp_send_request(uint32_t requested_ip, uint32_t server_ip) {
  if (!network_devices) {
    klog(LOG_WARN, "DHCP: No network device to send request.");
    return;
  }

  dhcp_packet_t pkt;
  memset(&pkt, 0, sizeof(pkt));

  pkt.op = DHCP_OP_REQUEST;
  pkt.htype = DHCP_HTYPE_ETHERNET;
  pkt.hlen = DHCP_HLEN_ETHERNET;
  pkt.xid = __builtin_bswap32(dhcp_xid);
  pkt.ciaddr = __builtin_bswap32(ip_get_local_ip()); // Client IP address if renewing, 0 if selecting
  pkt.flags = __builtin_bswap16(0x8000); // Broadcast flag
  memcpy(pkt.chaddr, network_devices->mac_addr, 6); // Client hardware address
  pkt.magic_cookie = __builtin_bswap32(DHCP_MAGIC_COOKIE);

  // Options
  uint8_t *opt_ptr = pkt.options;

  // DHCP Message Type: Request
  *opt_ptr++ = DHCP_OPTION_MSG_TYPE;
  *opt_ptr++ = 1;
  *opt_ptr++ = DHCP_MESSAGE_TYPE_REQUEST;

  // Requested IP Address
  *opt_ptr++ = DHCP_OPTION_REQ_IP_ADDR;
  *opt_ptr++ = 4;
  *((uint32_t*)opt_ptr) = __builtin_bswap32(requested_ip);
  opt_ptr += 4;

  // DHCP Server Identifier
  *opt_ptr++ = DHCP_OPTION_SERVER_ID;
  *opt_ptr++ = 4;
  *((uint32_t*)opt_ptr) = __builtin_bswap32(server_ip);
  opt_ptr += 4;

  *opt_ptr++ = DHCP_OPTION_END; // End option

  klog(LOG_INFO, "DHCP: Sending Request for %d.%d.%d.%d to server %d.%d.%d.%d",
       (requested_ip >> 24) & 0xFF, (requested_ip >> 16) & 0xFF, (requested_ip >> 8) & 0xFF, requested_ip & 0xFF,
       (server_ip >> 24) & 0xFF, (server_ip >> 16) & 0xFF, (server_ip >> 8) & 0xFF, server_ip & 0xFF);

  udp_send_packet(0xFFFFFFFF, 68, 67, (uint8_t *)&pkt, sizeof(pkt));
  current_dhcp_state = DHCP_STATE_REQUESTING;
}

void dhcp_send_request(uint32_t requested_ip, uint32_t server_ip); // Forward declaration

void dhcp_handle_reply(net_dev_t *net_dev, ipv4_header_t *ip_hdr,
                       udp_header_t *udp_hdr, const uint8_t *data, size_t len) {
  (void)net_dev;
  (void)ip_hdr;
  (void)udp_hdr;

  if (len < sizeof(dhcp_packet_t)) {
    klog(LOG_WARN, "DHCP: Received packet too small.");
    return;
  }

  dhcp_packet_t *pkt = (dhcp_packet_t *)data;

  // Validate magic cookie and transaction ID
  if (__builtin_bswap32(pkt->magic_cookie) != DHCP_MAGIC_COOKIE ||
      __builtin_bswap32(pkt->xid) != dhcp_xid) {
    klog(LOG_WARN, "DHCP: Invalid magic cookie or XID in reply.");
    return;
  }

  uint8_t msg_type_len;
  const uint8_t *msg_type_val;
  if (!get_dhcp_option(pkt->options, sizeof(pkt->options), DHCP_OPTION_MSG_TYPE,
                       &msg_type_len, &msg_type_val) ||
      msg_type_len != 1) {
    klog(LOG_WARN, "DHCP: DHCP Message Type option missing or invalid.");
    return;
  }

  uint8_t dhcp_msg_type = *msg_type_val;
  char ip_buf[16]; // For logging IP addresses

  switch (dhcp_msg_type) {
  case DHCP_MESSAGE_TYPE_OFFER:
    if (current_dhcp_state == DHCP_STATE_INIT || current_dhcp_state == DHCP_STATE_SELECTING) {
      offered_ip_address = __builtin_bswap32(pkt->yiaddr);
      dhcp_server_ip = __builtin_bswap32(pkt->siaddr); // Server IP from DHCP header

      uint8_t len_opt;
      const uint8_t *val_opt;

      // Parse subnet mask
      if (get_dhcp_option(pkt->options, sizeof(pkt->options), DHCP_OPTION_SUBNET_MASK, &len_opt, &val_opt) && len_opt == 4) {
          offered_subnet_mask = __builtin_bswap32(*(uint32_t*)val_opt);
      } else {
          offered_subnet_mask = 0; // Default to 0 if not provided
      }
      // Parse router
      if (get_dhcp_option(pkt->options, sizeof(pkt->options), DHCP_OPTION_ROUTER, &len_opt, &val_opt) && len_opt >= 4) {
          offered_router = __builtin_bswap32(*(uint32_t*)val_opt); // Take first router if multiple
      } else {
          offered_router = 0; // Default to 0
      }
      // Parse DNS server (currently stores only one)
      if (get_dhcp_option(pkt->options, sizeof(pkt->options), DHCP_OPTION_DNS_SERVER, &len_opt, &val_opt) && len_opt >= 4) {
          offered_dns_server = __builtin_bswap32(*(uint32_t*)val_opt); // Take first DNS if multiple
      } else {
          offered_dns_server = 0; // Default to 0
      }
      // Parse Server ID (from options, overrides siaddr if present)
      if (get_dhcp_option(pkt->options, sizeof(pkt->options), DHCP_OPTION_SERVER_ID, &len_opt, &val_opt) && len_opt == 4) {
          dhcp_server_ip = __builtin_bswap32(*(uint32_t*)val_opt);
      }
      // Parse Lease Time
      if (get_dhcp_option(pkt->options, sizeof(pkt->options), DHCP_OPTION_LEASE_TIME, &len_opt, &val_opt) && len_opt == 4) {
          lease_time = __builtin_bswap32(*(uint32_t*)val_opt);
          renewal_time = lease_time / 2;
          rebind_time = lease_time * 7 / 8;
      } else {
          lease_time = 0; // No lease time
      }


      ksprintf(ip_buf, "%d.%d.%d.%d", (offered_ip_address >> 24) & 0xFF, (offered_ip_address >> 16) & 0xFF, (offered_ip_address >> 8) & 0xFF, offered_ip_address & 0xFF);
      klog(LOG_INFO, "DHCP: Received OFFER: IP=%s, Server=%s", ip_buf, ksprintf(ip_buf, "%d.%d.%d.%d", (dhcp_server_ip >> 24) & 0xFF, (dhcp_server_ip >> 16) & 0xFF, (dhcp_server_ip >> 8) & 0xFF, dhcp_server_ip & 0xFF));

      current_dhcp_state = DHCP_STATE_SELECTING;
      dhcp_send_request(offered_ip_address, dhcp_server_ip);
    }
    break;

  case DHCP_MESSAGE_TYPE_ACK:
    if (current_dhcp_state == DHCP_STATE_REQUESTING) {
        if (__builtin_bswap32(pkt->yiaddr) == offered_ip_address) {
            // Confirmation of IP address. Now apply configuration.
            ip_set_local_ip(offered_ip_address);
            ip_set_subnet_mask(offered_subnet_mask);
            ip_set_default_gateway(offered_router);

            current_dhcp_state = DHCP_STATE_BOUND;
            ksprintf(ip_buf, "%d.%d.%d.%d", (offered_ip_address >> 24) & 0xFF, (offered_ip_address >> 16) & 0xFF, (offered_ip_address >> 8) & 0xFF, offered_ip_address & 0xFF);
            klog(LOG_INFO, "DHCP: Successfully BOUND to IP: %s", ip_buf);
        } else {
            klog(LOG_WARN, "DHCP: ACK for different IP than requested. Restarting discovery.");
            // Revert to initial state and restart discovery
            current_dhcp_state = DHCP_STATE_INIT;
            dhcp_discover();
        }
    }
    break;

  case DHCP_MESSAGE_TYPE_NAK:
    if (current_dhcp_state == DHCP_STATE_REQUESTING) {
        klog(LOG_WARN, "DHCP: Received NAK. Restarting discovery.");
        current_dhcp_state = DHCP_STATE_INIT;
        dhcp_discover();
    }
    break;

  default:
    klog(LOG_INFO, "DHCP: Received unhandled message type: %d", dhcp_msg_type);
    break;
  }
}

void dhcp_discover() {
  current_dhcp_state = DHCP_STATE_INIT;
  klog(LOG_INFO, "DHCP: Starting discovery process.");

  dhcp_packet_t pkt;
  memset(&pkt, 0, sizeof(pkt));

  pkt.op = DHCP_OP_REQUEST;
  pkt.htype = DHCP_HTYPE_ETHERNET;
  pkt.hlen = DHCP_HLEN_ETHERNET;
  pkt.xid = __builtin_bswap32(dhcp_xid);
  pkt.ciaddr = 0; // Client IP address - 0 for discover
  pkt.flags = __builtin_bswap16(0x8000); // Broadcast flag
  memcpy(pkt.chaddr, network_devices->mac_addr, 6); // Client hardware address
  pkt.magic_cookie = __builtin_bswap32(DHCP_MAGIC_COOKIE);

  // Options: 53 (DHCP Message Type), len 1, value 1 (Discover)
  pkt.options[0] = DHCP_OPTION_MSG_TYPE;
  pkt.options[1] = 1;
  pkt.options[2] = DHCP_MESSAGE_TYPE_DISCOVER;
  pkt.options[3] = DHCP_OPTION_END; // End option

  klog(LOG_INFO, "DHCP: Sending Discover...");
  udp_send_packet(0xFFFFFFFF, 68, 67, (uint8_t *)&pkt, sizeof(pkt));
}

void dhcp_init() {
  udp_register_handler(68, dhcp_handle_reply);
  klog(LOG_INFO, "DHCP client initialized.");
}


// i love dhcp so much
// dhcp is life
// dhcp is love
// dhcp forever
// dhcp is the rizzler
// dhcp is the skibidi toilet
// dhcp is the real mvp
// dhcp is the goat
// dhcp is the king
// dhcp is the boss
// dhcp is the legend
// dhcp is the champ
// dhcp is the hero
// dhcp is the savior
// dhcp is the rizzler toilet
// dhcp is the skibidi rizzler toilet
// dhcp is the ultimate rizzler toilet
// dhcp is the supreme rizzler toilet
// dhcp is the one true rizzler toilet
// dhcp is the rizzler toilet god
// dhcp is the rizzler toilet king
// dhcp is the rizzler toilet legend
// fuck PPPoE
// fuck L2TP
// fuck PPTP
// fuck ipv6
// fuck everything
// love dhcp