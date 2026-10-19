#!/usr/bin/env python3
"""Create the standalone Collision Lab without replacing any existing files."""
import argparse
from pathlib import Path
import re
import shutil
import sys
SOURCE=Path(__file__).resolve().parents[1]/'assets/collision-lab'
def create(destination,project_id='collision',title='COLLISION LAB'):
    if not re.fullmatch(r'[a-z][a-z0-9_]{0,10}',project_id):
        raise ValueError('Project ID needs 1-11 lowercase letters/digits/underscores, starting with a letter')
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9 _-]{0,47}',title):
        raise ValueError('Title needs 1-48 letters/digits/spaces/underscores/hyphens')
    destination=Path(destination).resolve()
    if destination.exists():raise ValueError('Choose a new folder; existing projects are never replaced')
    if destination.is_relative_to(SOURCE):raise ValueError('Output cannot be inside the source Lab')
    shutil.copytree(SOURCE,destination,ignore=shutil.ignore_patterns("build","disc","__pycache__","*.pyc"))
    for folder in ['build','disc','res']:(destination/folder).mkdir(exist_ok=True)
    make=destination/'Makefile'
    make.write_text(make.read_text().replace('@PROJECT_ID@',project_id).replace('@PROJECT_NAME@',title))
    return destination
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('destination',type=Path)
    parser.add_argument('--id',default='collision');parser.add_argument('--title',default='COLLISION LAB')
    args=parser.parse_args()
    try:print(create(args.destination,args.id,args.title))
    except (ValueError,OSError) as error:print(error,file=sys.stderr);raise SystemExit(2)
