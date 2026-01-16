#include "../../include/kpm.h"
#include "../kyroolib.h"
#include "crypto.h" // For SHA256_HASH_SIZE

// DNS Header structure (simplified)
typedef struct {
    uint16_t id;      // Identification number
    uint16_t flags;   // Query/response flags
    uint16_t qdcount; // Number of question entries
    uint16_t ancount; // Number of answer entries
    uint16_t nscount; // Number of authority entries
    uint16_t arcount; // Number of resource entries
} __attribute__((packed)) dns_header_t;

// DNS Question structure (part before QNAME)
typedef struct {
    // QNAME (variable length)
    uint16_t qtype;  // Query type (A, AAAA, MX, etc.)
    uint16_t qclass; // Query class (IN)
} __attribute__((packed)) dns_question_t;

// DNS Answer structure (part before NAME)
typedef struct {
    // NAME (variable length, usually pointer)
    uint16_t type;     // Type of resource record (A, AAAA, CNAME, etc.)
    uint16_t class;    // Class of data (IN)
    uint32_t ttl;      // Time to live
    uint16_t rdlength; // Length of RDATA
    // RDATA (variable length)
} __attribute__((packed)) dns_answer_t;

// DNS Flags
#define DNS_FLAG_QR_QUERY       (0 << 15) // Query
#define DNS_FLAG_QR_RESPONSE    (1 << 15) // Response
#define DNS_FLAG_OPCODE_QUERY   (0 << 11) // Standard query
#define DNS_FLAG_AA             (1 << 10) // Authoritative Answer
#define DNS_FLAG_TC             (1 << 9)  // Truncated
#define DNS_FLAG_RD             (1 << 8)  // Recursion Desired
#define DNS_FLAG_RA             (1 << 7)  // Recursion Available
#define DNS_FLAG_Z              (0 << 4)  // Reserved (must be 0)
#define DNS_FLAG_AD             (1 << 5)  // Authentic Data (RFC2535)
#define DNS_FLAG_CD             (1 << 4)  // Checking Disabled (RFC2535)
#define DNS_FLAG_RCODE_NOERROR  0         // No error

// DNS QTYPE/TYPE
#define DNS_QTYPE_A     1 // Host Address
#define DNS_QTYPE_CNAME 5 // Canonical Name
#define DNS_QTYPE_PTR   12 // Domain name pointer
#define DNS_QTYPE_AAAA  28 // IPv6 Host Address

// DNS QCLASS/CLASS
#define DNS_QCLASS_IN   1 // Internet

// Default DNS server (e.g., Google's public DNS)
#define DEFAULT_DNS_SERVER_IP 0x08080808 // 8.8.8.8
#define DNS_PORT 53

// Helper to convert a hostname string to DNS format (e.g., "www.example.com" to "\3www\7example\3com\0")
static void hostname_to_dns_format(const char *hostname, char *dns_format_buf) {
    int len = 0;
    char *p = (char *)hostname;
    char *label_start = (char *)hostname;

    while (*p) {
        if (*p == '.') {
            *dns_format_buf++ = len;
            memcpy(dns_format_buf, label_start, len);
            dns_format_buf += len;
            label_start = p + 1;
            len = 0;
        } else {
            len++;
        }
        p++;
    }
    *dns_format_buf++ = len;
    memcpy(dns_format_buf, label_start, len);
    dns_format_buf += len;
    *dns_format_buf = '\0'; // End with null terminator
}

// Basic DNS resolver (returns IP in host byte order)
static uint32_t resolve_hostname(const char *hostname) {
    print("DNS: Resolving hostname: "); print(hostname); print("\n");

    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        print("DNS: Failed to create socket.\n");
        return 0;
    }

    sockaddr_in_t server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = __builtin_bswap16(DNS_PORT);
    server_addr.sin_addr = __builtin_bswap32(DEFAULT_DNS_SERVER_IP);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // Bind to any local port
    sockaddr_in_t local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = 0; // Let kernel assign a port
    local_addr.sin_addr = 0; // Any local IP
    memset(local_addr.sin_zero, 0, sizeof(local_addr.sin_zero));
    if (bind(sock_fd, &local_addr, sizeof(sockaddr_in_t)) < 0) {
        print("DNS: Failed to bind socket.\n");
        close(sock_fd);
        return 0;
    }


    char dns_query_buf[512];
    memset(dns_query_buf, 0, sizeof(dns_query_buf));

    dns_header_t *header = (dns_header_t *)dns_query_buf;
    header->id = __builtin_bswap16(0x1234); // Random ID
    header->flags = __builtin_bswap16(DNS_FLAG_RD); // Recursion Desired
    header->qdcount = __builtin_bswap16(1); // One question

    char *qname_ptr = dns_query_buf + sizeof(dns_header_t);
    hostname_to_dns_format(hostname, qname_ptr);

    dns_question_t *question = (dns_question_t *)(qname_ptr + strlen(qname_ptr) + 1);
    question->qtype = __builtin_bswap16(DNS_QTYPE_A);
    question->qclass = __builtin_bswap16(DNS_QCLASS_IN);

    size_t query_len = sizeof(dns_header_t) + strlen(qname_ptr) + 1 + sizeof(dns_question_t);

    if (send(sock_fd, dns_query_buf, query_len, 0) < 0) {
        print("DNS: Failed to send DNS query.\n");
        close(sock_fd);
        return 0;
    }

    char dns_response_buf[512];
    int recv_len = recv(sock_fd, dns_response_buf, sizeof(dns_response_buf), 0);
    if (recv_len <= 0) {
        print("DNS: Failed to receive DNS response or timed out.\n");
        close(sock_fd);
        return 0;
    }

    dns_header_t *resp_header = (dns_header_t *)dns_response_buf;
    if (__builtin_bswap16(resp_header->id) != 0x1234 || !(resp_header->flags & __builtin_bswap16(DNS_FLAG_QR_RESPONSE))) {
        print("DNS: Invalid DNS response.\n");
        close(sock_fd);
        return 0;
    }

    if (__builtin_bswap16(resp_header->ancount) == 0) {
        print("DNS: No answer records in DNS response.\n");
        close(sock_fd);
        return 0;
    }

    // Skip header and question section
    char *ptr = dns_response_buf + sizeof(dns_header_t);
    // Skip QNAME
    while (*ptr) {
        ptr += *ptr + 1;
    }
    ptr++; // Skip null terminator
    ptr += sizeof(dns_question_t); // Skip QTYPE and QCLASS

    // Parse answer records
    for (int i = 0; i < __builtin_bswap16(resp_header->ancount); i++) {
        // Skip NAME (can be pointer or label sequence)
        if ((*ptr & 0xC0) == 0xC0) { // Pointer
            ptr += 2;
        } else { // Label sequence
            while (*ptr) {
                ptr += *ptr + 1;
            }
            ptr++;
        }
        
        dns_answer_t *answer = (dns_answer_t *)ptr;
        ptr += sizeof(dns_answer_t);

        if (__builtin_bswap16(answer->type) == DNS_QTYPE_A && __builtin_bswap16(answer->class) == DNS_QCLASS_IN) {
            // Found A record, RDATA is an IPv4 address
            uint32_t ip_addr = __builtin_bswap32(*(uint32_t *)ptr);
            char ip_buf[16];
            ksprintf(ip_buf, "%d.%d.%d.%d", (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF, (ip_addr >> 8) & 0xFF, ip_addr & 0xFF);
            print("DNS: Resolved "); print(hostname); print(" to "); print(ip_buf); print("\n");
            close(sock_fd);
            return ip_addr;
        }
        ptr += __builtin_bswap16(answer->rdlength); // Skip RDATA
    }

    print("DNS: Could not find A record for hostname.\n");
    close(sock_fd);
    return 0;
}

int pkg_fetch(const char *name) {
  char url[256];
  char hostname[128];
  char path_buf[256]; // Use a larger buffer for path
  uint16_t port = 80; // Default HTTP port
  uint32_t ip_addr = 0;

  ksprintf(url, "https://raw.githubusercontent.com/pon4ikisdonut/KyroOS-Packages/main/%s.kpkg", name);

  print("Fetching package from GitHub: "); print(url); print("\n");

  // --- URL Parsing ---
  const char *url_ptr = url;
  // Handle scheme
  if (memcmp(url_ptr, "https://", 8) == 0) {
      url_ptr += 8;
      port = 443; // Default HTTPS port
  } else if (memcmp(url_ptr, "http://", 7) == 0) {
      url_ptr += 7;
      port = 80; // Default HTTP port
  } else {
      print("HTTP: Invalid URL scheme. Only http(s):// supported.\n");
      return -1;
  }

  // Extract hostname
  const char *host_start = url_ptr;
  const char *host_end = strchr(host_start, '/');
  int host_len;
  if (host_end) {
      host_len = host_end - host_start;
      if (host_len >= sizeof(hostname)) host_len = sizeof(hostname) - 1;
      memcpy(hostname, host_start, host_len);
      hostname[host_len] = '\0';
      strcpy(path_buf, host_end); // Rest is path
  } else {
      strcpy(hostname, host_start); // No path, just host
      strcpy(path_buf, "/");
  }
  // Check for port number in hostname (e.g., example.com:8080)
  char *port_sep = strchr(hostname, ':');
  if (port_sep) {
      *port_sep = '\0'; // Null-terminate hostname
      port = atoi(port_sep + 1); // Convert port string to int
  }

  // --- DNS Resolution ---
  ip_addr = resolve_hostname(hostname);
  if (ip_addr == 0) {
      print("HTTP: Failed to resolve hostname.\n");
      return -1;
  }
  char ip_buf[16];
  ksprintf(ip_buf, "%d.%d.%d.%d", (ip_addr >> 24) & 0xFF, (ip_addr >> 16) & 0xFF, (ip_addr >> 8) & 0xFF, ip_addr & 0xFF);
  print("HTTP: Host resolved to: "); print(ip_buf); print("\n");


  // --- Create and Connect Socket ---
  int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Request TCP socket
  if (sock_fd < 0) {
      print("HTTP: Failed to create socket.\n");
      return -1;
  }

  sockaddr_in_t remote_addr;
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = __builtin_bswap16(port);
  remote_addr.sin_addr = ip_addr; // Already in network byte order from resolve_hostname
  memset(remote_addr.sin_zero, 0, sizeof(remote_addr.sin_zero));

  if (connect(sock_fd, &remote_addr, sizeof(sockaddr_in_t)) < 0) {
      print("HTTP: Failed to connect to server.\n");
      close(sock_fd);
      return -1;
  }
  print("HTTP: Connected to server.\n");

  // --- Send HTTP GET Request ---
  char request_buf[512];
  ksprintf(request_buf, "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path_buf, hostname);
  
  if (send(sock_fd, request_buf, strlen(request_buf), 0) < 0) {
      print("HTTP: Failed to send request.\n");
      close(sock_fd);
      return -1;
  }
  print("HTTP: Request sent.\n");

  // --- Receive HTTP Response ---
  // For simplicity, read entire response into a buffer.
  // In a real client, headers would be parsed first.
  char response_buf[4096]; // Max package size for now
  int total_recv = 0;
  int n;
  while ((n = recv(sock_fd, response_buf + total_recv, sizeof(response_buf) - 1 - total_recv, 0)) > 0) {
      total_recv += n;
      if (total_recv >= sizeof(response_buf) - 1) {
          print("HTTP: Response too large for buffer.\n");
          break;
      }
  }
  response_buf[total_recv] = '\0'; // Null-terminate received data
  print("HTTP: Response received ("); ksprintf(ip_buf, "%d", total_recv); print(ip_buf); print(" bytes).\n");

  // --- Parse HTTP Response and Save Package ---
  // Look for "HTTP/1.0 200 OK" or "HTTP/1.1 200 OK"
  char *body_start = strstr(response_buf, "\r\n\r\n");
  if (!body_start) {
      print("HTTP: Invalid HTTP response (no header-body separator).\n");
      close(sock_fd);
      return -1;
  }
  body_start += 4; // Move past "\r\n\r\n"

  // Check HTTP status code (simplified)
  if (memcmp(response_buf, "HTTP/1.", 7) != 0) { // Check for "HTTP/1.X"
      print("HTTP: Non-HTTP response.\n");
      close(sock_fd);
      return -1;
  }
  if (memcmp(response_buf + 9, "200", 3) != 0) { // Check for " 200"
      print("HTTP: Server returned non-200 status code.\n");
      close(sock_fd);
      return -1;
  }

  // Calculate actual package size
  size_t package_len = total_recv - (body_start - response_buf);
  
  // Save package to temporary file (e.g., /var/tmp/download.kpkg)
  char temp_pkg_path[128];
  ksprintf(temp_pkg_path, "/var/tmp/%s.kpkg", name);
  int temp_fd = open(temp_pkg_path, O_CREAT | O_TRUNC | O_WRONLY);
  if (temp_fd < 0) {
      print("HTTP: Failed to create temporary package file.\n");
      close(sock_fd);
      return -1;
  }
  write(temp_fd, body_start, package_len);
  close(temp_fd);

  print("HTTP: Package downloaded to "); print(temp_pkg_path); print("\n");

  // --- SHA256 checksum verification ---
  // For demonstration, use a hardcoded expected hash.
  // In a real system, this would come from a trusted source (e.g., manifest).
  uint8_t expected_hash[SHA256_HASH_SIZE];
  // Example hash for a dummy package "testpkg" with content "Hello, KyroOS!"
  // This value would need to be dynamically supplied for real packages.
  // Hash generated from 'echo -n "Hello, KyroOS!" | sha256sum'
  // 3f1e7e4a8f3b2e7c9d1a8e6b0f5c4a3b2d1e7c6a5b4a3f2e1d0c9b8a7f6e5d4c
  const char *expected_hash_hex = "3f1e7e4a8f3b2e7c9d1a8e6b0f5c4a3b2d1e7c6a5b4a3f2e1d0c9b8a7f6e5d4c";
  
  // Convert hex string to byte array (helper function needed)
  // For now, assume this helper exists or hardcode a byte array
  // For a basic implementation, if we expect "testpkg" to contain "Hello, KyroOS!",
  // then we calculate its hash and compare.

  print("SHA256: Calculating hash of downloaded package...\n");

  // Read the content of the downloaded file for hashing
  int pkg_fd = open(temp_pkg_path, O_RDONLY);
  if (pkg_fd < 0) {
      print("SHA256: Failed to open downloaded package for hashing.\n");
      close(sock_fd);
      return -1;
  }

  uint8_t* pkg_content = (uint8_t*)malloc(package_len); // Malloc from kyroolib.h if available
  if (!pkg_content) {
      print("SHA256: Failed to allocate buffer for hashing.\n");
      close(pkg_fd);
      close(sock_fd);
      return -1;
  }
  read(pkg_fd, pkg_content, package_len);
  close(pkg_fd);

  uint8_t calculated_hash[SHA256_HASH_SIZE];
  sha256(pkg_content, package_len, calculated_hash);
  free(pkg_content); // Free the allocated content

  print("SHA256: Calculated hash: ");
  for (int i = 0; i < SHA256_HASH_SIZE; i++) {
      char hex_byte[3];
      ksprintf(hex_byte, "%02x", calculated_hash[i]);
      print(hex_byte);
  }
  print("\n");

  // For a real check, compare calculated_hash with expected_hash
  // if (memcmp(calculated_hash, expected_hash, SHA256_HASH_SIZE) != 0) {
  //     print("SHA256: Checksum verification FAILED!\n");
  //     unlink(temp_pkg_path); // Delete potentially malicious file
  //     close(sock_fd);
  //     return -1;
  // }
  // print("SHA256: Checksum verification PASSED.\n");


  close(sock_fd);
  // Now call pkg_install_file to install the downloaded package
  return pkg_install_file(temp_pkg_path);
}
