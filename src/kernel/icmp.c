#include "icmp.h"
#include "ip.h"
#include "net.h"
#include "log.h"
#include "heap.h"
#include "kstring.h" // For memcpy, memset
// debian users, dont text me pls
static uint16_t icmp_echo_id = 0;
static uint16_t icmp_echo_sequence = 0;

// Simple checksum calculation (works with byte arrays for packed structs)
static uint16_t icmp_checksum(const void* data, size_t len) {
    const uint16_t* u16_buf = (const uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *u16_buf++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(const uint8_t*)u16_buf;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

void icmp_init() {
    ip_register_protocol_handler(IP_PROTOCOL_ICMP, icmp_handle_packet);
    klog(LOG_INFO, "ICMP initialized.");
}

void icmp_send_echo_request(net_dev_t* net_dev, uint32_t dest_ip, uint16_t id, uint16_t sequence, const uint8_t* data, size_t data_size) {
    (void)icmp_echo_id;       // Suppress warning
    (void)icmp_echo_sequence; // Suppress warning

    size_t icmp_packet_size = sizeof(icmp_header_t) + data_size;
    uint8_t* icmp_buffer = (uint8_t*)kmalloc(icmp_packet_size);
    if (!icmp_buffer) {
        klog(LOG_ERROR, "ICMP: Failed to allocate packet buffer for echo request.");
        return;
    }
    memset(icmp_buffer, 0, icmp_packet_size);

    icmp_header_t* icmp_hdr = (icmp_header_t*)icmp_buffer;
    uint8_t* icmp_data = icmp_buffer + sizeof(icmp_header_t);

    icmp_hdr->type = ICMP_ECHO_REQUEST;
    icmp_hdr->code = 0;
    icmp_hdr->id = __builtin_bswap16(id);
    icmp_hdr->sequence = __builtin_bswap16(sequence);
    memcpy(icmp_data, data, data_size);
    icmp_hdr->checksum = 0; // Clear for checksum calculation
    icmp_hdr->checksum = icmp_checksum(icmp_buffer, icmp_packet_size);

    ip_send_packet(net_dev, dest_ip, IP_PROTOCOL_ICMP, icmp_buffer, icmp_packet_size);
    kfree(icmp_buffer);
}

void icmp_handle_packet(net_dev_t* net_dev, const ipv4_header_t* ip_hdr, const uint8_t* payload, size_t payload_size) {
    if (payload_size < sizeof(icmp_header_t)) {
        klog(LOG_WARN, "ICMP: Payload too small.");
        return;
    }

    const icmp_header_t* icmp_hdr = (const icmp_header_t*)payload;
    
    // Verify checksum
    if (icmp_checksum(payload, payload_size) != 0) {
        klog(LOG_WARN, "ICMP: Invalid checksum.");
        return;
    }
    
    switch (icmp_hdr->type) {
        case ICMP_ECHO_REQUEST: {
            klog(LOG_INFO, "ICMP: Received Echo Request, sending reply.");
            
            size_t reply_packet_size = payload_size;
            uint8_t* reply_buffer = (uint8_t*)kmalloc(reply_packet_size);
            if (!reply_buffer) {
                klog(LOG_ERROR, "ICMP: Failed to allocate reply buffer.");
                return;
            }
            memcpy(reply_buffer, payload, payload_size); // Copy original request data

            icmp_header_t* icmp_reply_hdr = (icmp_header_t*)reply_buffer;
            icmp_reply_hdr->type = ICMP_ECHO_REPLY;
            icmp_reply_hdr->checksum = 0; // Recalculate checksum
            icmp_reply_hdr->checksum = icmp_checksum(reply_buffer, reply_packet_size);

            ip_send_packet(net_dev, __builtin_bswap32(ip_hdr->src_ip), IP_PROTOCOL_ICMP, reply_buffer, reply_packet_size);
            kfree(reply_buffer);
            break;
        }
        case ICMP_ECHO_REPLY: {
            klog(LOG_INFO, "ICMP: Received Echo Reply.");
            // Further processing for ping utility (e.g., matching ID/sequence, calculating RTT)
            break;
        }
        default: {
            klog(LOG_WARN, "ICMP: Unhandled ICMP type.");
            break;
        }
    }
}
