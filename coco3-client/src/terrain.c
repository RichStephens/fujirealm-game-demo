#include "terrain.h"
#include <cmoc.h>

void terrain_init(struct terrain_cache *cache, unsigned origin_x,
                  unsigned origin_y)
{
    cache->origin_x = origin_x;
    cache->origin_y = origin_y;
    cache->revision = 0;
    cache->revision_trust_next = 0;
    cache->commit_pending = 0;
    cache->commit_fill_id = 0;
}

static unsigned next_revision(unsigned revision)
{
    unsigned next = (revision + 1) & 0xFFFFU;

    return next == 0 ? 1U : next;
}

static signed char apply_edge_column(struct terrain_cache *cache,
                                     unsigned char origin_x,
                                     unsigned char origin_y,
                                     const unsigned char *tiles)
{
    unsigned char row;
    unsigned char *line;

    if (origin_y != cache->origin_y) {
        return TERRAIN_EDGE_RESYNC;
    }
    if (origin_x == cache->origin_x - 1) {
        for (row = 0; row < BOOTSTRAP_WINDOW_H; ++row) {
            line = &cache->tiles[(unsigned)row * BOOTSTRAP_WINDOW_W];
            memmove(line + 1, line, BOOTSTRAP_WINDOW_W - 1);
            line[0] = tiles[row];
        }
        --cache->origin_x;
        return TERRAIN_EDGE_APPLIED;
    }
    if ((unsigned)origin_x == cache->origin_x + BOOTSTRAP_WINDOW_W) {
        for (row = 0; row < BOOTSTRAP_WINDOW_H; ++row) {
            line = &cache->tiles[(unsigned)row * BOOTSTRAP_WINDOW_W];
            memmove(line, line + 1, BOOTSTRAP_WINDOW_W - 1);
            line[BOOTSTRAP_WINDOW_W - 1] = tiles[row];
        }
        ++cache->origin_x;
        return TERRAIN_EDGE_APPLIED;
    }
    return TERRAIN_EDGE_RESYNC;
}

static signed char apply_edge_row(struct terrain_cache *cache,
                                  unsigned char origin_x,
                                  unsigned char origin_y,
                                  const unsigned char *tiles)
{
    if (origin_x != cache->origin_x) {
        return TERRAIN_EDGE_RESYNC;
    }
    if (origin_y == cache->origin_y - 1) {
        memmove(&cache->tiles[BOOTSTRAP_WINDOW_W], &cache->tiles[0],
                (unsigned)(BOOTSTRAP_WINDOW_H - 1) * BOOTSTRAP_WINDOW_W);
        memcpy(&cache->tiles[0], tiles, BOOTSTRAP_WINDOW_W);
        --cache->origin_y;
        return TERRAIN_EDGE_APPLIED;
    }
    if ((unsigned)origin_y == cache->origin_y + BOOTSTRAP_WINDOW_H) {
        memmove(&cache->tiles[0], &cache->tiles[BOOTSTRAP_WINDOW_W],
                (unsigned)(BOOTSTRAP_WINDOW_H - 1) * BOOTSTRAP_WINDOW_W);
        memcpy(&cache->tiles[(unsigned)(BOOTSTRAP_WINDOW_H - 1) *
                             BOOTSTRAP_WINDOW_W],
               tiles, BOOTSTRAP_WINDOW_W);
        ++cache->origin_y;
        return TERRAIN_EDGE_APPLIED;
    }
    return TERRAIN_EDGE_RESYNC;
}

signed char terrain_apply_edge(struct terrain_cache *cache,
                               unsigned char origin_x, unsigned char origin_y,
                               unsigned char width, unsigned char height,
                               unsigned revision, const unsigned char *tiles,
                               unsigned char tile_count)
{
    signed char result;

    if (revision == cache->revision) {
        return TERRAIN_EDGE_DUPLICATE;
    }
    if (!cache->revision_trust_next &&
        revision != next_revision(cache->revision)) {
        return TERRAIN_EDGE_RESYNC;
    }

    if (width == 1 && height == BOOTSTRAP_WINDOW_H &&
        tile_count == BOOTSTRAP_WINDOW_H) {
        result = apply_edge_column(cache, origin_x, origin_y, tiles);
    } else if (height == 1 && width == BOOTSTRAP_WINDOW_W &&
               tile_count == BOOTSTRAP_WINDOW_W) {
        result = apply_edge_row(cache, origin_x, origin_y, tiles);
    } else {
        result = TERRAIN_EDGE_RESYNC;
    }

    if (result == TERRAIN_EDGE_APPLIED) {
        cache->revision = revision;
        cache->revision_trust_next = 0;
    }
    return result;
}

void terrain_fill_init(struct terrain_fill *fill)
{
    fill->active = 0;
    fill->fill_id = 0;
    fill->origin_x = 0;
    fill->origin_y = 0;
    fill->rows_have = 0;
}

signed char terrain_fill_apply_row(struct terrain_fill *fill,
                                   unsigned char fill_id,
                                   unsigned char origin_x,
                                   unsigned char abs_origin_y,
                                   unsigned char row_index,
                                   const unsigned char *tiles)
{
    unsigned fill_origin_y;
    unsigned long bit;

    if (row_index >= BOOTSTRAP_WINDOW_H) {
        return TERRAIN_FILL_IGNORED;
    }
    fill_origin_y = (unsigned)abs_origin_y - row_index;

    if (!fill->active || fill->fill_id != fill_id ||
        fill->origin_x != origin_x || fill->origin_y != fill_origin_y) {
        fill->active = 1;
        fill->fill_id = fill_id;
        fill->origin_x = origin_x;
        fill->origin_y = fill_origin_y;
        fill->rows_have = 0;
    }

    bit = 1UL << row_index;
    if (fill->rows_have & bit) {
        return TERRAIN_FILL_DUPLICATE;
    }
    memcpy(&fill->tiles[(unsigned)row_index * BOOTSTRAP_WINDOW_W], tiles,
           BOOTSTRAP_WINDOW_W);
    fill->rows_have |= bit;
    return fill->rows_have == TERRAIN_ALL_ROWS ? TERRAIN_FILL_COMPLETE
                                                : TERRAIN_FILL_ROW_APPLIED;
}

void terrain_fill_activate(struct terrain_fill *fill,
                           struct terrain_cache *cache)
{
    memcpy(cache->tiles, fill->tiles, BOOTSTRAP_TERRAIN_SIZE);
    cache->origin_x = fill->origin_x;
    cache->origin_y = fill->origin_y;
    cache->revision_trust_next = 1;
    cache->commit_pending = 1;
    cache->commit_fill_id = fill->fill_id;
    fill->active = 0;
}

unsigned char terrain_commit_ack_matches(const struct terrain_cache *cache,
                                         unsigned char fill_id,
                                         unsigned char origin_x,
                                         unsigned char origin_y)
{
    return cache->commit_pending && cache->commit_fill_id == fill_id &&
           cache->origin_x == origin_x && cache->origin_y == origin_y;
}
