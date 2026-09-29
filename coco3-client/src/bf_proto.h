#ifndef BF_PROTO_H
#define BF_PROTO_H

/* $BF-framed packet codec shared by the login exchange (LOGIN_SERVER_PORT)
 * and the bootstrap exchange (HYBRID_SERVER_PORT): MAGIC VERSION type len
 * payload... sum8, where the checksum is the low byte of the sum of every
 * preceding byte. Matches server/protocol.py's encode_packet/decode_packet
 * exactly. Ported from lynx-client/src/bootstrap.c's bf_parser. */

#define BF_MAGIC 0xBF
#define BF_VERSION 1

/* Big enough for a WELCOME (5 bytes) or a WINDOW chunk (12-byte header +
 * 96 tiles = 108). */
#define BF_MAX_PAYLOAD 108
#define BF_MAX_FRAME (BF_MAX_PAYLOAD + 5)

struct bf_packet {
    unsigned char type;
    unsigned char payload_len;
    unsigned char payload[BF_MAX_PAYLOAD];
};

struct bf_parser {
    unsigned char frame[BF_MAX_FRAME];
    unsigned char pos;
    unsigned char want;
};

void bf_parser_init(struct bf_parser *parser);

/* Feeds one stream byte. Returns 1 and fills *packet when a complete,
 * checksum-valid frame lands; 0 otherwise. A bad version or checksum byte
 * silently resyncs the parser (the caller just keeps feeding). */
unsigned char bf_parser_feed(struct bf_parser *parser, unsigned char byte,
                             struct bf_packet *packet);

/* Builds a complete frame (magic+ver+type+len+payload+sum8) into out.
 * out must hold payload_len + 5 bytes. Returns the frame length. */
unsigned char bf_build(unsigned char *out, unsigned char type,
                       const unsigned char *payload, unsigned char payload_len);

#endif
