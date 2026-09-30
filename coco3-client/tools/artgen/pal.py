# Palettes as (r,g,b) levels 0-3 per slot; GIME RGB code bits R1 G1 B1 R0 G0 B0.
def code(r,g,b): return ((r>>1)<<5)|((g>>1)<<4)|((b>>1)<<3)|((r&1)<<2)|((g&1)<<1)|(b&1)
LEGEND = '.dgwGelmbtBrcoyp'   # slot 0..15
OVERWORLD = [(0,0,0),(1,1,1),(2,2,2),(3,3,3),(0,1,0),(1,2,0),(2,3,1),(1,0,0),
             (2,1,0),(3,2,1),(0,0,2),(3,0,0),(1,2,3),(2,2,0),(3,3,0),(2,0,2)]
# Cave: the grass greens become stone, so grass-backed tiles sit on cave floor;
# light green stays, for herbs and moss.
CAVE = list(OVERWORLD)
CAVE[4]=(0,0,0); CAVE[5]=(1,1,1)
# PvP realm: dead grass, dark water.
PVP = list(OVERWORLD)
PVP[4]=(1,0,0); PVP[5]=(1,1,0); PVP[10]=(1,0,1)
PALETTES = {'overworld':OVERWORLD,'cave':CAVE,'pvp':PVP}
def hexcol(c): return tuple(v*85 for v in c)

# Composite codes (intensity*16 + hue) approximating the RGB colors.
COMPOSITE = {(0,0,0):0,(1,1,1):16,(2,2,2):32,(3,3,3):48,(0,1,0):15,(1,2,0):31,
    (2,3,1):47,(1,0,0):7,(2,1,0):6,(3,2,1):37,(0,0,2):12,(3,0,0):23,(1,2,3):44,
    (2,2,0):20,(3,3,0):36,(2,0,2):25,(1,1,0):4,(1,0,1):9}
