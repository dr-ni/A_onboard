#!/usr/bin/python
from distutils.core import setup
import glob
setup(
    name = 'onboard',
    author = 'Chris Jones',
    author_email = 'cej105@soton.ac.uk',
    maintainer = 'Ubuntu Core Developers',
    maintainer_email = 'ubuntu-devel-discuss@lists.ubuntu.com',
    url = 'http://launchpad.net/onboard/',
    license = 'gpl',
    description = 'Simple On-screen Keyboard',
    packages = ['Onboard'],
    data_files = [('share/onboard/data', glob.glob('data/*')),
                 ('share/onboard/layouts', glob.glob('layouts/*')),
                 ('share/onboard/scripts', glob.glob('scripts/*'))],
    scripts = ['onboard', 'onboard-settings']
)

