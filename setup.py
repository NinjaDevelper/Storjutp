#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2015, Shinya Yagyu
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from setuptools.command.test import test as TestCommand
import os
import sys
from distutils.core import setup, Extension

LONG_DESCRIPTION = open('README.md').read()
VERSION = '1.0'

install_requirements = []

test_requirements = [
    'pytest>=2.6.4',
    'pytest-pep8',
    'pytest-cache',
    'coveralls'
]

libutp_src = [
    'libutp/utp_utils.cpp',
    'libutp/utp_api.cpp',
    'libutp/utp_callbacks.cpp',
    'libutp/utp_hash.cpp',
    'libutp/utp_packedsockaddr.cpp',
    'libutp/utp_internal.cpp',
]

src = ['cxx/utpbinder_python.cpp','cxx/FileInfo.cpp',
       'cxx/Storjutp.cpp']

rt = []
if sys.platform != 'darwin':
        rt = ['-lrt']

module = Extension('storjutp.utpbinder', src + libutp_src, 
                   extra_link_args=rt,
                   extra_compile_args=['-DPOSIX'],
                   include_dirs=['libutp',
                                 'cxx'],
                   )


class PyTest(TestCommand):
    user_options = [('pytest-args=', 'a', "Arguments to pass to py.test")]

    def initialize_options(self):
        TestCommand.initialize_options(self)
        self.pytest_args = []

    def finalize_options(self):
        TestCommand.finalize_options(self)
        self.test_args = []
        self.test_suite = True

    def run_tests(self):
        # Import PyTest here because outside, the eggs are not loaded.
        import pytest
        import sys
        errno = pytest.main(self.pytest_args)
        sys.exit(errno)

setup(
    name='storjutp',
    version=VERSION,
    url='https://github.com/StorjPlatform/Storjutp',
    download_url='https://github.com/StorjPlatform/Storjutp/tarball/'
    + VERSION,
    license=open('LICENSE').read(),
    author='Shinya Yagyu',
    author_email='utamaro.sisho@gmail.com',
    description='File Transfer Library by uTP.',
    long_description=LONG_DESCRIPTION,
    packages=['storjutp'],
    cmdclass={'test': PyTest},
    ext_modules=[module],
    install_requires=install_requirements,
    tests_require=test_requirements,
    keywords=['storj', 'storj platform', 'uTP' 'LEDBAT']
)
