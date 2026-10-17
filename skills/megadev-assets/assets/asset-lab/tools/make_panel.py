"""Optional exact 8x8 UI fill, sharing the resident sprite palette in this Lab."""
import json
from pathlib import Path
from PIL import Image
r=Path(__file__).resolve().parents[1]/'res'
palette=json.loads((r/'actor.palette.json').read_text())
color=tuple(bytes.fromhex(palette[1][1:]))
Image.new('RGBA',(8,8),(*color,255)).save(r/'panel.png',compress_level=9)
(r/'panel.palette.json').write_text(json.dumps(palette,indent=2)+'\n')
