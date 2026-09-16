#include <string.h>
#include "gm_ring.h"

#define REC_PAYLOAD_LEN 28u /* rs_rec_t serialized size (packed below) */
#define SLOT_LEN 32u        /* payload + crc32 */

typedef struct {
    uint32_t magic;
    uint32_t slot_count;
    uint32_t seq_next;
    uint32_t written_total;
    uint32_t crc; /* over the preceding 16 bytes */
} rs_hdr_t;

uint32_t rs_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320u & (~(crc & 1) + 1u));
    }
    return ~crc;
}

static void put32(uint8_t *p, uint32_t v) {
    p[0]=v; p[1]=v>>8; p[2]=v>>16; p[3]=v>>24;
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static void put64(uint8_t *p, uint64_t v) { put32(p,(uint32_t)v); put32(p+4,(uint32_t)(v>>32)); }
static uint64_t get64(const uint8_t *p) { return (uint64_t)get32(p) | ((uint64_t)get32(p+4)<<32); }

static void rec_ser(const rs_rec_t *r, uint8_t *p)
{
    put32(p+0, r->seq);
    put32(p+4, r->started_epoch);
    put32(p+8, r->elapsed_s);
    put64(p+12, r->dist_mm);
    put32(p+20, (uint32_t)r->avg_rpm | ((uint32_t)r->max_rpm << 16));
    put32(p+24, (uint32_t)r->estimate_basis);
}
static void rec_des(rs_rec_t *r, const uint8_t *p)
{
    r->seq = get32(p+0);
    r->started_epoch = get32(p+4);
    r->elapsed_s = get32(p+8);
    r->dist_mm = get64(p+12);
    uint32_t v = get32(p+20);
    r->avg_rpm = (uint16_t)v;
    r->max_rpm = (uint16_t)(v>>16);
    r->estimate_basis = (uint8_t)get32(p+24);
}

static int hdr_write(rs_t *rs)
{
    uint8_t b[20];
    rs_hdr_t h = { RS_MAGIC, rs->slot_count, rs->seq_next, rs->written_total, 0 };
    put32(b+0,h.magic); put32(b+4,h.slot_count); put32(b+8,h.seq_next); put32(b+12,h.written_total);
    put32(b+16, rs_crc32(b, 16));
    return rs->io.write(rs->io.user, 0, b, 20) == 1 ? 0 : -1;
}

static int hdr_read(rs_t *rs, rs_hdr_t *h)
{
    uint8_t b[20];
    if (rs->io.read(rs->io.user, 0, b, 20) != 1) return -1;
    h->magic = get32(b+0); h->slot_count = get32(b+4);
    h->seq_next = get32(b+8); h->written_total = get32(b+12);
    if (h->magic != RS_MAGIC) return -1;
    if (get32(b+16) != rs_crc32(b, 16)) return -1;
    return 0;
}

static uint32_t slot_off(const rs_t *rs, uint32_t idx)
{
    (void)rs;
    return 32u /* header region rounded up to a slot */ + idx * SLOT_LEN;
}

int rs_open(rs_t *rs, const rs_io_t *io)
{
    memset(rs, 0, sizeof(*rs));
    rs->io = *io;
    rs_hdr_t h;
    if (hdr_read(rs, &h) != 0 || h.slot_count == 0 ||
        h.slot_count > (io->capacity - 32u) / SLOT_LEN) {
        /* format a fresh ring (also handles blank flash 0xFFFFFFFF) */
        rs->slot_count = (io->capacity - 32u) / SLOT_LEN;
        rs->seq_next = 1;
        return hdr_write(rs);
    }
    rs->slot_count = h.slot_count;
    rs->seq_next = h.seq_next;
    rs->written_total = h.written_total;
    /* Scan every slot: CRC-valid -> valid, non-blank-but-bad -> dropped. */
    uint8_t buf[SLOT_LEN];
    for (uint32_t i = 0; i < rs->slot_count; i++) {
        if (rs->io.read(rs->io.user, slot_off(rs, i), buf, SLOT_LEN) != 1)
            return -1;
        int blank = 1;
        for (uint32_t j = 0; j < SLOT_LEN; j++) if (buf[j] != 0xFF && buf[j] != 0x00) { blank = 0; break; }
        if (blank) continue;
        if (get32(buf + REC_PAYLOAD_LEN) == rs_crc32(buf, REC_PAYLOAD_LEN))
            rs->slots_valid++;
        else
            rs->dropped++; /* torn tail / corruption: counted, never overwritten silently */
    }
    return 0;
}

int rs_append(rs_t *rs, const rs_rec_t *rec)
{
    rs_rec_t r = *rec;
    r.seq = rs->seq_next;
    uint8_t buf[SLOT_LEN];
    memset(buf, 0xFF, SLOT_LEN);
    rec_ser(&r, buf);
    put32(buf + REC_PAYLOAD_LEN, rs_crc32(buf, REC_PAYLOAD_LEN));
    uint32_t idx = rs->written_total % rs->slot_count;
    if (rs->io.write(rs->io.user, slot_off(rs, idx), buf, SLOT_LEN) != 1)
        return -1;
    rs->seq_next++;
    if (rs->written_total < rs->slot_count) rs->slots_valid++;
    rs->written_total++;
    return hdr_write(rs);
}

int rs_get(const rs_t *rs, uint32_t seq, rs_rec_t *out)
{
    if (seq == 0 || seq >= rs->seq_next) return -1;
    uint32_t age = rs->seq_next - 1 - seq; /* newest first */
    if (age >= rs->slot_count) return -1;  /* evicted by ring wrap */
    uint32_t idx = (rs->written_total - 1 - age) % rs->slot_count;
    uint8_t buf[SLOT_LEN];
    if (rs->io.read(rs->io.user, slot_off(rs, idx), buf, SLOT_LEN) != 1)
        return -1;
    if (get32(buf + REC_PAYLOAD_LEN) != rs_crc32(buf, REC_PAYLOAD_LEN))
        return -1;
    rec_des(out, buf);
    return (out->seq == seq) ? 0 : -1;
}

uint32_t rs_valid_count(const rs_t *rs) { return rs->slots_valid; }

uint32_t rs_slots_free(const rs_t *rs)
{
    return (rs->written_total >= rs->slot_count) ? 0u
                                                 : rs->slot_count - rs->written_total;
}

int rs_wipe(rs_t *rs)
{
    if (rs->io.erase && rs->io.erase(rs->io.user, 0, rs->io.capacity) != 1)
        return -1;
    uint8_t blank[SLOT_LEN];
    memset(blank, 0xFF, sizeof(blank));
    if (!rs->io.erase) {
        for (uint32_t i = 0; i < rs->slot_count; i++)
            if (rs->io.write(rs->io.user, slot_off(rs, i), blank, SLOT_LEN) != 1)
                return -1;
    }
    rs->slots_valid = 0;
    rs->dropped = 0;
    rs->written_total = 0;
    /* seq_next kept: sequence numbers are monotonic across wipes */
    return hdr_write(rs);
}
