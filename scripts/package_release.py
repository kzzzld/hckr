#!/usr/bin/env python3
import argparse
import gzip
import hashlib
from pathlib import Path
import re
import subprocess


def release_version(tag):
    if not re.fullmatch(r'v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)', tag):
        raise ValueError('Use a stable version tag such as v0.1.0')
    return tag[1:]


def package_release(tag, output):
    version = release_version(tag)
    source = subprocess.check_output([
        'git', 'archive', '--format=tar', f'--prefix=hckr-{version}/', tag,
    ])
    output.mkdir(parents=True, exist_ok=True)
    archive = output / f'hckr-{version}.tar.gz'
    with archive.open('wb') as stream:
        with gzip.GzipFile(filename='', mode='wb', fileobj=stream, mtime=0) as compressed:
            compressed.write(source)
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output / 'SHA256SUMS').write_text(f'{checksum}  {archive.name}\n')
    return archive


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Package a tagged hckr source release')
    parser.add_argument('tag')
    parser.add_argument('--output', type=Path, default=Path('dist'))
    args = parser.parse_args()
    try:
        print(package_release(args.tag, args.output))
    except (ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'{error}\n')
