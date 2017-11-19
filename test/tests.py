#!/usr/bin/env python
from __future__ import print_function, unicode_literals
from colorprint import *
from PIL import Image
from ssim import compute_ssim
import os, sys, subprocess

test_dir = os.path.dirname(os.path.realpath(__file__))

build_dir = '../build'
tifig_bin = os.path.join(build_dir, 'tifig')
fixtures_dir = '../fixtures'
references_dir = 'iOS11_ref_exports'

ssim_min_value = 0.90

def run(command):
    return subprocess.call(command, shell=True)

def build_tifig():
    if os.path.exists(tifig_bin):
        print('> tifig binary found', color='green')
        return

    print('Building tifig', color='yellow')

    if not os.path.isdir(build_dir):
        os.mkdir(build_dir)

    os.chdir(build_dir)

    cmake_ret = run('cmake ..')
    make_ret =run('make')

    if not (cmake_ret == 0 and make_ret == 0 and os.path.exists(tifig_bin)):
        print('Failed to build tifig', color='red')
        exit(1)

    os.chdir(test_dir)

def get_test_candidates():
    fixtures = sorted(os.listdir(fixtures_dir))
    references = sorted(os.listdir(references_dir))

    for heic, ref_jpg in zip(fixtures, references):
        heic_path = os.path.join(fixtures_dir, heic)
        ref_path = os.path.join(references_dir, ref_jpg)
        yield((heic_path, ref_path, ref_jpg))


def run_similarity_test():
    for heic, ref, converted in get_test_candidates():
        print('> Converting %s => %s' % (os.path.basename(heic), converted), color='yellow')
        tifig_ret = run('%s -v %s %s' % (tifig_bin, heic, converted))
        if (tifig_ret != 0):
            print('Converting heic image with tifig failed!', color='red')
            exit(1)

        print('> Computing visual similarity', color='cyan')
        
        ref_img = Image.open(ref)
        converted_img = Image.open(converted)

        ssim = compute_ssim(ref_img, converted_img)

        if (ssim < ssim_min_value):
            print('Similarity: %2.2f%%' % (ssim * 100), color='red')
            print('This is below our acceptance rate of %2f%%!' % ssim_min_value, color='red')
            exit(1)
        else:
            print('Similarity: %2.2f%%' % (ssim * 100), color='green')

        os.remove(converted)

        print()


if __name__ == "__main__":
    print()
    print('#######################################', color='blue')
    print('# Running tifig regression test suite #', color='blue')
    print('#######################################', color='blue')
    print()

    # make sure this is callable from everywhere
    os.chdir(test_dir)

    build_tifig()
    print()
    run_similarity_test()
