#!/usr/bin/env python3
"""Copy the original cross-skill validation game into a new disposable project."""
from pathlib import Path
import argparse
import re
import shutil
SOURCE=Path(__file__).resolve().parent/'glasshouse'
def create(destination,project_id='glasshouse',title='GLASSHOUSE AFTER HOURS'):
 if not re.fullmatch(r'[a-z][a-z0-9_]{0,10}',project_id):raise ValueError('Invalid project ID')
 if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 _-]{0,47}',title):raise ValueError('Invalid title')
 destination=Path(destination).resolve()
 if destination.exists() or destination.is_relative_to(SOURCE):raise ValueError('Choose a new folder outside the source game')
 shutil.copytree(SOURCE,destination,ignore=shutil.ignore_patterns('build','disc','__pycache__','*.pyc','*.wav','*.pcm'))
 for name in ['build','disc']:(destination/name).mkdir()
 p=destination/'Makefile';p.write_text(p.read_text().replace('@PROJECT_ID@',project_id).replace('@PROJECT_NAME@',title))
 return destination
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('destination',type=Path);a=p.parse_args();print(create(a.destination))
