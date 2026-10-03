#include "bf_proto.h"

static unsigned char sum8(const unsigned char *buf, unsigned char len)
{
    unsigned char i;
    unsigned char sum = 0;

    for (i = 0; i < len; ++i) {
        sum += buf[i];
    }
    return sum;
}

void bf_parser_init(struct bf_parser *parser)
{
    parser->pos = 0;
    parser->want = 4;
}

unsigned char bf_parser_feed(struct bf_parser *parser, unsigned char byte,
                             struct bf_packet *packet)
{
    unsigned char i;

    if (parser->pos == 0 && byte != BF_MAGIC) {
        return 0;
    }

    parser->frame[parser->pos++] = byte;

    if (parser->pos == 2 && parser->frame[1] != BF_VERSION) {
        bf_parser_init(parser);
        return 0;
    }
    if (parser->pos == 4) {
        if (parser->frame[3] > BF_MAX_PAYLOAD) {
            bf_parser_init(parser);
            return 0;
        }
        parser->want = (unsigned char)(parser->frame[3] + 5);
    }
    if (parser->pos < parser->want) {
        return 0;
    }
    if (sum8(parser->frame, (unsigned char)(parser->want - 1)) !=
        parser->frame[parser->want - 1]) {
        bf_parser_init(parser);
        return 0;
    }

    packet->type = parser->frame[2];
    packet->payload_len = parser->frame[3];
    for (i = 0; i < packet->payload_len; ++i) {
        packet->payload[i] = parser->frame[4 + i];
    }
    bf_parser_init(parser);
    return 1;
}

unsigned char bf_build(unsigned char *out, unsigned char type,
                       const unsigned char *payload, unsigned char payload_len)
{
    unsigned char i;

    out[0] = BF_MAGIC;
    out[1] = BF_VERSION;
    out[2] = type;
    out[3] = payload_len;
    for (i = 0; i < payload_len; ++i) {
        out[4 + i] = payload[i];
    }
    out[4 + payload_len] = sum8(out, (unsigned char)(4 + payload_len));
    return (unsigned char)(5 + payload_len);
}
