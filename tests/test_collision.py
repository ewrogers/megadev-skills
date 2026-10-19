"""Exact rational oracle, bounded sliding and real base-M68000 collision contracts."""
from fractions import Fraction
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import random
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
LAB=ROOT/'skills/megadev-collision/assets/collision-lab'
FIXTURE=LAB/'tools'

def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    sys.path.insert(0,str(path.parent))
    try:spec.loader.exec_module(module)
    finally:sys.path.pop(0)
    return module

def compile_host(cc,binary,mode,lab):
    args=[cc,'-DCC_HOST','-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(lab/'src'),'-I',str(FIXTURE),
        str(lab/'src/collision.c'),str(lab/'src/demo.c'),str(lab/'tools/test_collision.c'),'-o',str(binary)]
    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
    result=subprocess.run(args,capture_output=True,text=True,timeout=60)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)

def oracle(row):
    ax,ay,aw,ah,dx,dy,bx,by,bw,bh,vx,vy,point=row
    if point:aw=ah=0;vx=vy=0
    if ax<bx+bw and ax+aw>bx and ay<by+bh and ay+ah>by:return (2,0,0,0,None)
    entry=[];leave=[];normals=[]
    for p,size,lo,hi,d,axis in [(ax,aw,bx,bx+bw,dx-vx,0),(ay,ah,by,by+bh,dy-vy,1)]:
        if d==0:
            if p>=hi or p+size<=lo:return (0,0,0,0,None)
            continue
        ends=sorted([Fraction(lo-p-size,d),Fraction(hi-p,d)])
        entry.append(ends[0]);leave.append(ends[1]);normals.append((ends[0],axis,-1 if d>0 else 1))
    if not entry:return (0,0,0,0,None)
    t=max(entry)
    if t<0 or t>1 or t>=min(leave):return (0,0,0,0,None)
    ns=[0,0]
    for at,axis,normal in normals:
        if at==t:ns[axis]=normal
    return (1,int(t*4096),*ns,t)

class CollisionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cls.cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
            raise unittest.SkipTest('C17 compiler required')
        cls.scratch=tempfile.TemporaryDirectory(prefix='collision contracts ');cls.addClassCleanup(cls.scratch.cleanup)
        cls.lab=Path(cls.scratch.name)/'game';shutil.copytree(LAB,cls.lab)
        subprocess.run([sys.executable,str(cls.lab/'tools/make_art.py')],check=True,stdout=subprocess.DEVNULL)
        cls.binaries=[]
        for mode in ['-O0','-O3']:
            binary=Path(cls.scratch.name)/(mode+('.exe' if os.name=='nt' else ''))
            compile_host(cls.cc,binary,mode,cls.lab);cls.binaries.append(binary)

    def case(self,name):
        for binary in self.binaries:
            r=subprocess.run([str(binary),name],capture_output=True,text=True,timeout=30)
            self.assertEqual(r.returncode,0,r.stderr)

    def test_sweeps_relative_motion_edge_contact_corner_grazing_and_range(self):self.case('sweep')
    def test_segment_parallel_boundary_inside_and_zero_length(self):self.case('ray')
    def test_nearest_exact_time_slide_corner_ties_and_initial_overlap(self):self.case('slide')
    def test_invalid_geometry_motion_pointer_and_candidate_capacity_fail_explicitly(self):self.case('invalid')
    def test_pal_ntsc_pause_and_long_slide_route(self):self.case('demo')

    def test_12000_queries_against_independent_fraction_oracle(self):
        rng=random.Random(80000);rows=[]
        for i in range(12000):
            ax,ay,bx,by=[rng.randint(-16384,16384) for _ in range(4)]
            if i%3:bx=ax+rng.randint(-1800,1800);by=ay+rng.randint(-1800,1800)
            bx=max(-16384,min(16384,bx));by=max(-16384,min(16384,by))
            aw,ah,bw,bh=[rng.randint(1,4096) for _ in range(4)]
            dx,dy,vx,vy=[rng.randint(-4096,4096) for _ in range(4)]
            dx=max(-16384-ax,min(16384-ax,dx));dy=max(-16384-ay,min(16384-ay,dy))
            vx=max(-16384-bx,min(16384-bx,vx));vy=max(-16384-by,min(16384-by,vy))
            rows.append((ax,ay,aw,ah,dx,dy,bx,by,bw,bh,vx,vy,i%2))
        data=''.join(' '.join(map(str,row))+'\n' for row in rows)
        expected=[oracle(row) for row in rows]
        self.assertGreater(sum(e[0]==1 for e in expected),300)
        self.assertGreater(sum(e[0]==2 for e in expected),300)
        for binary in self.binaries:
            r=subprocess.run([str(binary),'query'],input=data,capture_output=True,text=True,timeout=30)
            self.assertEqual(r.returncode,0,r.stderr)
            lines=r.stdout.splitlines();self.assertEqual(len(lines),len(rows))
            for row,line,want in zip(rows,lines,expected):
                got=list(map(int,line.split()));self.assertEqual(got[:4],list(want[:4]),row)
                if got[0]==1:self.assertEqual(Fraction(got[4],got[5]),want[4],row)


class CollisionAssetsTests(unittest.TestCase):
    def test_standalone_art_and_shared_wall_geometry(self):
        creator=load('collision_creator',ROOT/'skills/megadev-collision/scripts/create_project.py')
        with tempfile.TemporaryDirectory() as folder:
            game=creator.create(Path(folder)/'collision game')
            with self.assertRaises(ValueError):creator.create(game)
            art=load('collision_art',game/'tools/make_art.py')
            data,areas,count=art.build()
            self.assertEqual(data,art.build()[0]);self.assertLessEqual(count,256)
            import struct
            self.assertEqual(len(data['actors.bin']),13*32)
            self.assertEqual(len(data['maps.bin']),4*960*2)
            for name,value in data.items():self.assertEqual(value,(game/'res'/name).read_bytes())
            self.assertEqual(art.vdp(7,0,0),0xE);self.assertEqual(art.vdp(0,7,0),0xE0);self.assertEqual(art.vdp(0,0,7),0xE00)
            self.assertTrue(all(not(c&~0xEEE) for c in struct.unpack('>64H',data['palettes.bin'])))
            for area in areas:
                for y in range(32,224):
                    for x in range(320):
                        solid=any(bx<=x<bx+w and by<=y<by+h for bx,by,w,h in area['walls'])
                        self.assertEqual(art.pixel(area,x,y) in (6,8),solid,(x,y))
            subprocess.run([sys.executable,str(game/'tools/check_collision.py')],check=True,capture_output=True,text=True)
    def test_renderer_and_interrupt_owner_reused(self):
        for name in ['gfx.c','gfx.h','bridge.s']:
            self.assertEqual((LAB/'src'/name).read_bytes(),(ROOT/'skills/megadev-gameplay/assets/adventure-lab/src'/name).read_bytes())
