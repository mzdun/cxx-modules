#!/usr/bin/env python3

import os
import subprocess
import sys
import shutil

suite = {
    "01-cxx17": "app",
    "02-internal": "app",
    "03-strings-hm": "app",
    "03-strings-inc": "app",
    "04-impl": "app",
    "05-strings-B": "app",
    "06-static-lib": "app/example",
}

__dirname__ = os.path.dirname(__file__)

binary = os.path.join(os.getcwd(), 'bin', 'c++modules')

class cd:
    def __init__(self, dirname):
        self.dirname = os.path.expanduser(dirname)

    def __enter__(self):
        self.saved = os.getcwd()
        os.chdir(self.dirname)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.saved)


def run(*args, **kwargs):
    # print(*args)
    p = subprocess.Popen(args, **kwargs)
    p.communicate()
    if p.returncode != 0:
        print(args[0], 'ended with error', p.returncode, file=sys.stderr)
        return False
    return True


def run_test(dirname, application):
    print('==[   {:=<50}'.format(dirname + '   ]'))
    with cd(os.path.join(__dirname__, dirname)):
        shutil.rmtree('build', ignore_errors=True)
        if not run(binary):
            return
        with cd('build'):
            run('dot', '-Tpng', '-o', 'dependencies.png', 'dependencies.dot') and \
                run('ninja') and \
                run(os.path.join('.', application))


if len(sys.argv) > 1:
    for dirname in sys.argv[1:]:
        run_test(dirname, suite[dirname])
else:
    for dirname in sorted(suite.keys()):
        run_test(dirname, suite[dirname])
