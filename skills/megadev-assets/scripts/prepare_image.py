#!/usr/bin/env python3
"""Prepare one PNG without resizing or silently reducing its palette."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
from asset_codec import cram, rgb, pack, inspect, unpack


def prepare(image_path, palette_path, output, frame=None, snap=False, force=False, backdrop=False):
    from PIL import Image
    image_path,palette_path,output=map(Path,(image_path,palette_path,output))
    report_path=output.with_suffix('.asset.json');preview=output.with_suffix('.used.png')
    if not force and any(p.exists() for p in (output,report_path,preview)):
        raise ValueError('Output already exists; use --force to regenerate deliberately')
    if any(p.resolve() in (image_path.resolve(),palette_path.resolve()) for p in (output,report_path,preview)):
        raise ValueError('Output cannot replace source artwork')
    if frame and backdrop:raise ValueError('A sprite cannot use index zero as an opaque backdrop')
    raw=json.loads(palette_path.read_text(encoding='utf-8'))
    if not isinstance(raw,list) or len(raw)!=16 or any(not isinstance(c,str) or len(c)!=7 or c[0]!='#' for c in raw):
        raise ValueError('Palette must be a JSON array of exactly 16 #RRGGBB strings')
    colors=[tuple(bytes.fromhex(c[1:])) for c in raw];palette=[cram(c,snap) for c in colors]
    with Image.open(image_path) as source:
        if source.format!='PNG' or getattr(source,'n_frames',1)!=1:
            raise ValueError('Expected a static PNG; provide an animation as a frame grid')
        width,height=source.size
        if width>512 or height>512:
            raise ValueError('Prepare smaller sections; this helper limits input to 512x512')
        image=source.convert('RGBA')
        indices=[];snapped=set();opaque=set()
        for r,g,b,a in image.get_flattened_data():
            if a not in (0,255):raise ValueError('Partial alpha is unsupported; flatten or edit it explicitly')
            if a==0:indices.append(0);continue
            value=cram((r,g,b),snap);opaque.add(value)
            if rgb(value)!=(r,g,b):snapped.add((r,g,b))
            options=range(16) if backdrop else range(1,16)
            index=next((i for i in options if palette[i]==value),None)
            if index is None:raise ValueError('An opaque pixel is outside the supplied palette; no nearest-palette substitution is performed')
            indices.append(index)
    data=pack(indices,width,height,palette,frame);info=inspect(data)
    columns=width//frame[0] if frame else None
    pw,ph,pixels=unpack(data,columns)
    used=Image.new('RGBA',(pw,ph))
    used.putdata([(*rgb(palette[i]),0 if i==0 and not backdrop else 255) for i in pixels])
    output.parent.mkdir(parents=True,exist_ok=True)
    def item(path):return {'path':os.path.relpath(path,report_path.parent).replace(os.sep,'/'),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    output.write_bytes(data);used.save(preview)
    info.pop('mapping');info['palette']=[f'{x:04X}' for x in palette]
    report={'version':1,'inputs':[item(image_path),item(palette_path)],'outputs':[item(output),item(preview)],
            'source_size':[width,height],'frame':list(frame) if frame else None,'frame_columns':columns,
            'snap_enabled':snap,'index_zero_mode':'scene_backdrop' if backdrop else 'transparent','snapped_source_colors':len(snapped),'opaque_cram_colors':len(opaque),'asset':info}
    report_path.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('image',type=Path);p.add_argument('palette',type=Path);p.add_argument('output',type=Path)
    p.add_argument('--frame',nargs=2,type=int,metavar=('WIDTH','HEIGHT'));p.add_argument('--snap',action='store_true');p.add_argument('--backdrop',action='store_true',help='Index zero is the scene backdrop; runtime must match it');p.add_argument('--force',action='store_true');a=p.parse_args()
    try:
        report=prepare(a.image,a.palette,a.output,tuple(a.frame) if a.frame else None,a.snap,a.force,a.backdrop)
        print(json.dumps({'tiles':report['asset']['tiles'],'tile_bytes':report['asset']['tile_bytes'],'snapped_colors':report['snapped_source_colors']}))
    except (OSError,ValueError,ImportError) as error:
        print(str(error),file=sys.stderr);raise SystemExit(2)
