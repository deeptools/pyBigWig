#!/usr/bin/env python
from distutils.core import setup, Extension
import subprocess
import glob
import sys

srcs = [x for x in 
    glob.glob("libBigWig/*.c")]
srcs.append("pyBigWig.c")

libpython = "python%i.%i" % (sys.version_info[0], sys.version_info[1])

module1 = Extension('pyBigWig',
                    sources = srcs,
                    libraries = ["m", "z", "curl", libpython],
                    extra_compile_args = ["-O0"],
                    include_dirs = ['libBigWig'])

setup (name = 'pyBigWig',
       version = '1.0',
       description = 'A package for accessing bigWig files',
       ext_modules = [module1])
