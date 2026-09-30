"""Preview sheets (needs Pillow)."""
from PIL import Image, ImageDraw
import pal
def img(rows, palname='overworld', scale=8, bg=None):
    p=pal.PALETTES[palname]
    im=Image.new('RGB',(16*scale,16*scale),(40,40,40))
    d=ImageDraw.Draw(im)
    for y,r in enumerate(rows):
        for x,ch in enumerate(r):
            i=pal.LEGEND.index(ch)
            if i==0 and bg is not None:
                c=bg[y][x] if isinstance(bg,list) else bg
                i=pal.LEGEND.index(c) if isinstance(c,str) else None
                if i is None: continue
            d.rectangle([x*scale,y*scale,x*scale+scale-1,y*scale+scale-1],fill=pal.hexcol(p[i]))
    return im
def sheet(items, out, palname='overworld', tiled=False, bg=None, cols=4):
    scale=6; cell=16*scale
    w=cols*(cell*(3 if tiled else 1)+20); rows=(len(items)+cols-1)//cols
    h=rows*(cell*(3 if tiled else 1)+30)
    S=Image.new('RGB',(w,h),(30,30,30)); D=ImageDraw.Draw(S)
    for n,(name,r) in enumerate(items):
        cx=(n%cols)*(cell*(3 if tiled else 1)+20)+10; cy=(n//cols)*(cell*(3 if tiled else 1)+30)+5
        t=img(r,palname,scale,bg)
        if tiled:
            for a in range(3):
                for b in range(3): S.paste(t,(cx+a*cell,cy+b*cell))
        else: S.paste(t,(cx,cy))
        D.text((cx,cy+cell*(3 if tiled else 1)+2),name,fill=(255,255,255))
    S.save(out)
