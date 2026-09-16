/* Session ring storage over the 'storage' raw partition.
 * Flash write granularity: esp_partition_write handles 1-4 byte alignment;
 * our slot writes are 32-byte aligned so this is safe. Erase granularity is
 * 4 KiB sectors — the ring region is erased once at format/wipe only.
 * Power-loss safety comes from per-record CRC + header CRC (gm_ring), not
 * from the erase here; a torn record tail is detected and counted. */
#include <string.h>
#include "esp_partition.h"
#include "esp_log.h"
#include "board.h"
#include "gm_ring.h"
#include "storage_ring.h"

static const char *TAG = "ring_store";
static const esp_partition_t *s_part;

static int part_read(void *user, uint32_t off, uint8_t *buf, uint32_t len)
{
    (void)user;
    return esp_partition_read(s_part, off, buf, len) == ESP_OK ? 1 : 0;
}
static int part_write(void *user, uint32_t off, const uint8_t *buf, uint32_t len)
{
    (void)user;
    return esp_partition_write(s_part, off, buf, len) == ESP_OK ? 1 : 0;
}
static int part_erase(void *user, uint32_t off, uint32_t len)
{
    (void)user;
    return esp_partition_erase_range(s_part, off, len) == ESP_OK ? 1 : 0;
}

int ring_storage_open(rs_t *rs)
{
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                      ESP_PARTITION_SUBTYPE_ANY,
                                      RING_PARTITION);
    if (!s_part) { ESP_LOGE(TAG, "storage partition missing"); return -1; }
    rs_io_t io = {
        .read = part_read, .write = part_write, .erase = part_erase,
        .user = NULL, .capacity = s_part->size,
    };
    return rs_open(rs, &io);
}
