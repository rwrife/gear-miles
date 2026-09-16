/* Gear Miles — session ring store (pure C, host-testable).
 *
 * Append-only ring of fixed-size session records with a per-record CRC32
 * and a CRC'd header (firmware/README.md item 5, architecture §6 rows
 * "Session ring record CRC failure" and "Power loss mid-session").
 *
 * Storage is injected (read/write callbacks): host tests use a RAM
 * buffer; the device maps it onto a raw data partition.
 */
#ifndef GM_RING_H
#define GM_RING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS_MAGIC 0x474D5231u /* "GMR1" */

typedef struct {
    uint32_t seq;          /* monotonic u32 ring sequence (client key) */
    uint32_t started_epoch; /* epoch seconds, 0 = no time known */
    uint32_t elapsed_s;
    uint64_t dist_mm;
    uint16_t avg_rpm;
    uint16_t max_rpm;
    uint8_t  estimate_basis; /* always 1 = estimate per metric-honesty policy */
} rs_rec_t;

typedef struct {
    int (*read)(void *user, uint32_t off, uint8_t *buf, uint32_t len);
    int (*write)(void *user, uint32_t off, const uint8_t *buf, uint32_t len);
    /* Optional bulk clear (NOR flash erase semantics). Required for wipe on
     * real flash: writing 0xFF cannot restore erased state. Host tests may
     * leave NULL (RAM buffer can accept 0xFF byte writes directly). */
    int (*erase)(void *user, uint32_t off, uint32_t len);
    void *user;
    uint32_t capacity;     /* total bytes available for the ring */
} rs_io_t;

typedef struct {
    rs_io_t io;
    uint32_t slot_count;   /* capacity in records */
    uint32_t seq_next;     /* next sequence number to assign */
    uint32_t written_total;
    uint32_t dropped;      /* CRC-failed slots detected at open */
    uint32_t slots_valid;
} rs_t;

uint32_t rs_crc32(const uint8_t *data, uint32_t len); /* reflected CRC-32 */

/* Open (and format when the header is blank/corrupt). Scans all slots and
 * counts CRC-valid entries, incrementing `dropped` for torn records
 * (visible via /api/status as dropped_records — no silent overwrite).
 * Returns 0 on success.
 */
int rs_open(rs_t *rs, const rs_io_t *io);

/* Append a session; seq assigned from seq_next (wraps storage, not seq). */
int rs_append(rs_t *rs, const rs_rec_t *rec);

/* Read the record with the given sequence number if it is still in the
 * ring and CRC-valid. Returns 0 on success, -1 otherwise. */
int rs_get(const rs_t *rs, uint32_t seq, rs_rec_t *out);

/* Number of valid records currently in the ring. */
uint32_t rs_valid_count(const rs_t *rs);

/* Free record slots (0 when the ring is full — history_slots_free). */
uint32_t rs_slots_free(const rs_t *rs);

/* Erase all records (factory reset path): header re-formatted, seq kept. */
int rs_wipe(rs_t *rs);

#ifdef __cplusplus
}
#endif
#endif /* GM_RING_H */
