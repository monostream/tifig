#!/usr/bin/env python
from __future__ import print_function, unicode_literals
import os, sys, subprocess
from colorprint import *

test_dir = os.path.dirname(os.path.realpath(__file__))

build_dir = '../build'
tifig_bin = build_dir + '/tifig'
fixtures_dir = '../fixtures'

butteraugli_url = 'https://github.com/google/butteraugli/archive/master.tar.gz'
butteraugli_dir = 'butteraugli-master/butteraugli'
butteraugli_bin = 'butteraugli-master/butteraugli/butteraugli'

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

def build_butteraugli():

	if os.path.exists(butteraugli_bin):
		print('> butteraugli binary found', color='green')
		return

	print('Building butteraugli', color='yellow')

	run('wget --show-progress -qO- %s | tar xz' % butteraugli_url)

	os.chdir(butteraugli_dir)

	make_ret = run('make')

	if (make_ret != 0):
		print('Failed to build butteraugli', color='red')
		exit(1)

	os.chdir(test_dir)

def get_test_candidates():
	fixtures = os.listdir(fixtures_dir)
	basenames = {os.path.splitext(f)[0] for f in fixtures}
	for f in basenames:
		if f+'.heic' in fixtures and f+'_ref.jpg' in fixtures:
			heic = '%s/%s.heic' % (fixtures_dir, f)
			ref = '%s/%s_ref.jpg' % (fixtures_dir, f)
			converted = f + '_tifig.png'
			yield((heic, ref, converted, f))

def run_similarity_test():

	for heic, ref, converted, name in get_test_candidates():
		print('> Converting %s => %s' % (name + '.heic', converted), color='yellow')
		tifig_ret = run('%s -v %s %s' % (tifig_bin, heic, converted))
		if (tifig_ret != 0):
			print('Converting heic image with tifig failed!', color='red')
			exit(1)

		print('> Comparing visual similarity', color='cyan')
		run(' '.join([butteraugli_bin, ref, converted, name + '_heatmap.ppm']))

		print()


if __name__ == "__main__":

	print('#######################################', color='blue')
	print('# Running tifig integration test suit #', color='blue')
	print('#######################################', color='blue')
	print()

	# make sure this is callable from everywhere
	os.chdir(test_dir)

	build_tifig()
	build_butteraugli()
	print()
	run_similarity_test()
