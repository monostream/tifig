#!/usr/bin/env python
from __future__ import print_function, unicode_literals
import os, sys, subprocess, skimage.io, skimage.measure
from colorprint import *

test_dir = os.path.dirname(os.path.realpath(__file__))

build_dir = '../build'
tifig_bin = build_dir + '/tifig'
fixtures_dir = '../fixtures'

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
	fixtures = os.listdir(fixtures_dir)
	basenames = sorted({os.path.splitext(f)[0] for f in fixtures})
	for f in basenames:
		if f + '.heic' in fixtures and f + '.jpg' in fixtures:
			heic = '%s/%s.heic' % (fixtures_dir, f)
			ref = '%s/%s.jpg' % (fixtures_dir, f)
			converted = f + '_tifig.tiff'
			yield((heic, ref, converted, f))


def run_similarity_test():
	for heic, ref, converted, name in get_test_candidates():
		print('> Converting %s => %s' % (name + '.heic', converted), color='yellow')
		tifig_ret = run('%s -v %s %s' % (tifig_bin, heic, converted))
		if (tifig_ret != 0):
			print('Converting heic image with tifig failed!', color='red')
			exit(1)

		print('> Comparing visual similarity', color='cyan')
		
		ref_img = skimage.io.imread(ref)
		converted_img = skimage.io.imread(converted)

		ssim = skimage.measure.compare_ssim(ref_img, converted_img, multichannel=True)

		if (ssim < ssim_min_value):
			print('Similarity: %2.1f%%' % (ssim * 100), color='red')
			print('This below our acceptance rate of %.2f!' % ssim_min_value, color='red')
			exit(1)
		else:
			print('Similarity: %2.1f%%' % (ssim * 100), color='green')

		os.remove(converted)

		print()


if __name__ == "__main__":
	print()
	print('######################################', color='blue')
	print('# Running tifig regression test suit #', color='blue')
	print('######################################', color='blue')
	print()

	# make sure this is callable from everywhere
	os.chdir(test_dir)

	build_tifig()
	print()
	run_similarity_test()
