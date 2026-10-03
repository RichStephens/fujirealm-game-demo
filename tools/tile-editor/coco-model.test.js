const test = require('node:test');
const assert = require('node:assert');
const Coco = require('./coco-model.js');

function rows(fill = '0') { return Array.from({ length: 16 }, () => fill.repeat(16)); }

function makeTileset() {
    const codes = Array.from({ length: 16 }, (_, i) => i);
    return {
        version: 1,
        projectType: 'fujirealm-coco3-tiles',
        tileWidth: 16,
        tileHeight: 16,
        palettes: { overworld: codes.slice(), cave: codes.slice(), pvp: codes.slice() },
        tiles: Coco.LIVE_TILES.map(index => ({ index, name: `Tile ${index}`, rows: rows() })),
        players: Array.from({ length: Coco.PLAYER_COUNT }, (_, index) => ({ index, rows: rows() })),
        entities: [{ name: 'beaver', rows: rows() }],
    };
}

test('accepts a well-formed tileset', () => {
    assert.doesNotThrow(() => Coco.validateTileset(makeTileset()));
});

test('rejects the wrong project type, size and palettes', () => {
    const wrongType = makeTileset(); wrongType.projectType = 'fujirealm-lynx-tiles';
    assert.throws(() => Coco.validateTileset(wrongType), /projectType/);

    const wrongSize = makeTileset(); wrongSize.tileWidth = 8;
    assert.throws(() => Coco.validateTileset(wrongSize), /16x16/);

    const noCave = makeTileset(); delete noCave.palettes.cave;
    assert.throws(() => Coco.validateTileset(noCave), /palettes.cave/);

    const badCode = makeTileset(); badCode.palettes.pvp[3] = 64;
    assert.throws(() => Coco.validateTileset(badCode), /0-63/);
});

test('accepts only the live tile ids', () => {
    const legacy = makeTileset(); legacy.tiles[0].index = 1;
    assert.throws(() => Coco.validateTileset(legacy), /not a live tile id/);

    const short = makeTileset(); short.tiles.pop();
    assert.throws(() => Coco.validateTileset(short), /Expected 35 tiles/);

    const dup = makeTileset(); dup.tiles[1].index = 0;
    assert.throws(() => Coco.validateTileset(dup), /Duplicate tile index/);
});

test('rejects malformed pixel rows', () => {
    const shortRows = makeTileset(); shortRows.tiles[0].rows = rows().slice(1);
    assert.throws(() => Coco.validateTileset(shortRows), /16 rows/);

    const shortRow = makeTileset(); shortRow.players[0].rows[2] = '123';
    assert.throws(() => Coco.validateTileset(shortRow), /16 hex digits/);

    const notHex = makeTileset(); notHex.entities[0].rows[2] = '0123456789ABCDEZ';
    assert.throws(() => Coco.validateTileset(notHex), /non-hex/);
});

test('converts GIME RGB codes to browser colors', () => {
    assert.strictEqual(Coco.codeToHex(0), '#000000');
    assert.strictEqual(Coco.codeToHex(63), '#ffffff');
    assert.strictEqual(Coco.codeToHex(36), '#ff0000'); // R1 + R0
    assert.strictEqual(Coco.codeToHex(2), '#005500');  // G0 only
    assert.strictEqual(Coco.codeToHex(56), '#aaaaaa'); // R1 G1 B1
});

test('enumerates tiles, player frames and entities as one list', () => {
    const tileset = makeTileset();
    const list = Coco.entries(tileset);
    assert.strictEqual(list.length, Coco.LIVE_TILES.length + Coco.PLAYER_COUNT + 1);
    assert.strictEqual(list[0].key, 'tile:0');
    assert.strictEqual(list[0].overlay, false);
    assert.strictEqual(list[Coco.LIVE_TILES.length].key, 'player:0');
    assert.strictEqual(list[Coco.LIVE_TILES.length].overlay, true);
    assert.strictEqual(Coco.findEntry(tileset, 'entity:beaver').entry.name, 'beaver');
    assert.strictEqual(Coco.findEntry(tileset, 'tile:1'), null);
});

test('reads and writes pixels, reporting whether anything changed', () => {
    const entry = { rows: rows() };
    assert.strictEqual(Coco.setPixel(entry, 15, 2, 12), true);
    assert.strictEqual(Coco.getPixel(entry, 15, 2), 12);
    assert.strictEqual(entry.rows[2], '000000000000000C');
    assert.strictEqual(Coco.setPixel(entry, 15, 2, 12), false);
    assert.strictEqual(Coco.getPixel(entry, 16, 0), -1);
    assert.throws(() => Coco.setPixel(entry, 0, 0, 16), /0-15/);
});

test('transforms shift, mirror and clear', () => {
    const entry = { rows: rows() };
    Coco.setPixel(entry, 0, 0, 5);
    Coco.transform(entry, 'right');
    assert.strictEqual(Coco.getPixel(entry, 1, 0), 5);
    Coco.transform(entry, 'down');
    assert.strictEqual(Coco.getPixel(entry, 1, 1), 5);
    Coco.transform(entry, 'mirror-h');
    assert.strictEqual(Coco.getPixel(entry, 14, 1), 5);
    Coco.transform(entry, 'mirror-v');
    assert.strictEqual(Coco.getPixel(entry, 14, 14), 5);
    Coco.transform(entry, 'clear');
    assert.strictEqual(Coco.getPixel(entry, 14, 14), 0);
    assert.throws(() => Coco.transform(entry, 'sideways'), /Unknown transform/);
});

test('undo state round-trips every entry', () => {
    const tileset = makeTileset();
    const state = Coco.cloneState(tileset);
    Coco.setPixel(tileset.tiles[5], 1, 1, 9);
    Coco.setPixel(tileset.entities[0], 2, 2, 7);
    Coco.restoreState(tileset, state);
    assert.strictEqual(Coco.getPixel(tileset.tiles[5], 1, 1), 0);
    assert.strictEqual(Coco.getPixel(tileset.entities[0], 2, 2), 0);
});

test('packs the 128 bytes the game stores', () => {
    const entry = { rows: rows() };
    Coco.setPixel(entry, 0, 0, 2);
    Coco.setPixel(entry, 1, 0, 3);
    Coco.setPixel(entry, 15, 15, 9);
    const bytes = Coco.imageBytes(entry);
    assert.strictEqual(bytes.length, 128);
    assert.strictEqual(bytes[0], 0x23);   // leftmost pixel in the high nibble
    assert.strictEqual(bytes[127], 0x09);
});
