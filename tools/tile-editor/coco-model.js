(function (root, factory) {
    const api = factory();
    if (typeof module === 'object' && module.exports) module.exports = api;
    root.FujiRealmCoco = api;
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
    'use strict';

    // The CoCo 3 tileset: 16x16 pixels of palette indices (GIME 320x225x16),
    // stored as sixteen strings of sixteen hex digits. The palettes are the RGB
    // halves of coco3-client/src/palette.c, as GIME 6-bit codes; the editor
    // only previews with them, and tools/art_pack.py fails the build if they
    // drift from palette.c.
    const PROJECT_VERSION = 1;
    const PROJECT_TYPE = 'fujirealm-coco3-tiles';
    const TILE_W = 16;
    const TILE_H = 16;
    const PALETTE_SIZE = 16;
    // Front, right, left, back, each standing then stepping. Other players
    // use the same frames, recolored by the game.
    const PLAYER_COUNT = 8;
    const PLAYER_NAMES = ['Front', 'Front step', 'Right', 'Right step',
                          'Left', 'Left step', 'Back', 'Back step'];
    const PALETTE_NAMES = ['overworld', 'cave', 'pvp'];
    // Tile ids the terrain stream uses; 1 and 18-33 are dead legacy slots
    // (docs/TILE_ALLOCATION.md).
    const LIVE_TILES = [0].concat(range(2, 18), range(34, 52));
    // Color 0 is transparent on player frames and entities.
    const TRANSPARENT = 0;

    const HEX = '0123456789ABCDEF';

    function range(from, to) {
        const out = [];
        for (let i = from; i < to; i++) out.push(i);
        return out;
    }

    function fail(message) { throw new Error(message); }

    function validateRows(rows, label) {
        if (!Array.isArray(rows) || rows.length !== TILE_H) fail(`${label} must have ${TILE_H} rows`);
        rows.forEach((row, y) => {
            if (typeof row !== 'string' || row.length !== TILE_W) fail(`${label} row ${y} must be ${TILE_W} hex digits`);
            for (const ch of row) if (HEX.indexOf(ch.toUpperCase()) < 0) fail(`${label} row ${y} contains non-hex "${ch}"`);
        });
    }

    function validateTileset(tileset) {
        if (!tileset || typeof tileset !== 'object' || Array.isArray(tileset)) fail('Tileset root must be an object');
        if (tileset.projectType !== PROJECT_TYPE) fail(`Tileset projectType must be ${PROJECT_TYPE}`);
        if (tileset.version !== PROJECT_VERSION) fail(`Tileset version must be ${PROJECT_VERSION}`);
        if (tileset.tileWidth !== TILE_W || tileset.tileHeight !== TILE_H) fail('Only 16x16 tiles are supported');

        if (!tileset.palettes || typeof tileset.palettes !== 'object') fail('palettes must be an object');
        PALETTE_NAMES.forEach(name => {
            const codes = tileset.palettes[name];
            if (!Array.isArray(codes) || codes.length !== PALETTE_SIZE) fail(`palettes.${name} must hold ${PALETTE_SIZE} codes`);
            codes.forEach((code, i) => {
                if (!Number.isInteger(code) || code < 0 || code > 63) fail(`palettes.${name}[${i}] must be a GIME code 0-63`);
            });
        });

        if (!Array.isArray(tileset.tiles) || tileset.tiles.length !== LIVE_TILES.length) fail(`Expected ${LIVE_TILES.length} tiles`);
        const tileIndices = new Set();
        tileset.tiles.forEach(tile => {
            if (LIVE_TILES.indexOf(tile.index) < 0) fail(`Tile ${tile.index} is not a live tile id`);
            if (tileIndices.has(tile.index)) fail(`Duplicate tile index ${tile.index}`);
            tileIndices.add(tile.index);
            validateRows(tile.rows, `tile ${tile.index}`);
        });

        if (!Array.isArray(tileset.players) || tileset.players.length !== PLAYER_COUNT) fail(`Expected ${PLAYER_COUNT} player frames`);
        const playerIndices = new Set();
        tileset.players.forEach(frame => {
            if (!Number.isInteger(frame.index) || frame.index < 0 || frame.index >= PLAYER_COUNT) fail(`Player frame has an invalid index: ${frame.index}`);
            if (playerIndices.has(frame.index)) fail(`Duplicate player frame ${frame.index}`);
            playerIndices.add(frame.index);
            validateRows(frame.rows, `player ${frame.index}`);
        });

        if (!Array.isArray(tileset.entities) || !tileset.entities.length) fail('entities must be a non-empty array');
        const names = new Set();
        tileset.entities.forEach(entity => {
            if (typeof entity.name !== 'string' || !entity.name) fail('Every entity needs a name');
            if (names.has(entity.name)) fail(`Duplicate entity ${entity.name}`);
            names.add(entity.name);
            validateRows(entity.rows, `entity ${entity.name}`);
        });
        return tileset;
    }

    // GIME RGB code bits are R1 G1 B1 R0 G0 B0: two bits per channel.
    function codeToHex(code) {
        const level = (hi, lo) => ((code >> hi) & 1) * 2 + ((code >> lo) & 1);
        const channel = v => (v * 85).toString(16).padStart(2, '0');
        return `#${channel(level(5, 2))}${channel(level(4, 1))}${channel(level(3, 0))}`;
    }

    function paletteHex(tileset, name) {
        return tileset.palettes[name].map(codeToHex);
    }

    // One flat list so the library, selection and undo treat tiles, player
    // frames and entities alike. Players and entities draw over terrain.
    function entries(tileset) {
        const list = [];
        tileset.tiles.slice().sort((a, b) => a.index - b.index).forEach(tile => {
            list.push({ key: `tile:${tile.index}`, kind: 'tile', label: tile.name || `Tile ${tile.index}`, detail: `tile ${tile.index}`, overlay: false, entry: tile });
        });
        tileset.players.slice().sort((a, b) => a.index - b.index).forEach(frame => {
            list.push({ key: `player:${frame.index}`, kind: 'player', label: `Player ${PLAYER_NAMES[frame.index]}`, detail: `player ${frame.index}`, overlay: true, entry: frame });
        });
        tileset.entities.forEach(entity => {
            list.push({ key: `entity:${entity.name}`, kind: 'entity', label: entity.name.replace(/_/g, ' '), detail: `entity ${entity.name}`, overlay: true, entry: entity });
        });
        return list;
    }

    function findEntry(tileset, key) {
        return entries(tileset).find(item => item.key === key) || null;
    }

    function getPixel(entry, x, y) {
        if (x < 0 || x >= TILE_W || y < 0 || y >= TILE_H) return -1;
        return parseInt(entry.rows[y][x], 16);
    }

    function setPixel(entry, x, y, value) {
        if (x < 0 || x >= TILE_W || y < 0 || y >= TILE_H) return false;
        if (!Number.isInteger(value) || value < 0 || value >= PALETTE_SIZE) fail('Color must be 0-15');
        const row = entry.rows[y];
        if (row[x].toUpperCase() === HEX[value]) return false;
        entry.rows[y] = row.slice(0, x) + HEX[value] + row.slice(x + 1);
        return true;
    }

    function transform(entry, action) {
        const grid = entry.rows.map(row => row.split(''));
        let next;
        if (action === 'left') next = grid.map(row => row.slice(1).concat(row[0]));
        else if (action === 'right') next = grid.map(row => [row[row.length - 1]].concat(row.slice(0, -1)));
        else if (action === 'up') next = grid.slice(1).concat([grid[0]]);
        else if (action === 'down') next = [grid[grid.length - 1]].concat(grid.slice(0, -1));
        else if (action === 'mirror-h') next = grid.map(row => row.slice().reverse());
        else if (action === 'mirror-v') next = grid.slice().reverse();
        else if (action === 'clear') next = grid.map(row => row.map(() => HEX[TRANSPARENT]));
        else fail(`Unknown transform ${action}`);
        entry.rows = next.map(row => row.join(''));
        return entry.rows;
    }

    function cloneState(tileset) {
        return entries(tileset).map(item => ({ key: item.key, rows: item.entry.rows.slice() }));
    }

    function restoreState(tileset, state) {
        const byKey = new Map(entries(tileset).map(item => [item.key, item.entry]));
        state.forEach(saved => {
            const entry = byKey.get(saved.key);
            if (entry) entry.rows = saved.rows.slice();
        });
    }

    // The 128 bytes the game stores: two pixels per byte, leftmost in the
    // high nibble, rows top to bottom (tools/art_pack.py).
    function imageBytes(entry) {
        const out = [];
        entry.rows.forEach(row => {
            for (let x = 0; x < TILE_W; x += 2) {
                out.push((parseInt(row[x], 16) << 4) | parseInt(row[x + 1], 16));
            }
        });
        return out;
    }

    return {
        PROJECT_VERSION, PROJECT_TYPE, TILE_W, TILE_H, PALETTE_SIZE,
        PLAYER_COUNT, PALETTE_NAMES, LIVE_TILES, TRANSPARENT,
        validateTileset, codeToHex, paletteHex, entries, findEntry, getPixel,
        setPixel, transform, cloneState, restoreState, imageBytes,
    };
});
