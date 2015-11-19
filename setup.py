#!/usr/bin/env python
from distutils.core import setup, Extension
import subprocess
import glob
import sys

srcs = [x for x in 
    glob.glob("libBigWig/*.c")]
srcs.append("pyBigWig.c")

if(sys.version_info[0] >= 3) :
    libpython = "python%i.%im" % (sys.version_info[0], sys.version_info[1])
else :
    libpython = "python%i.%i" % (sys.version_info[0], sys.version_info[1])

module1 = Extension('pyBigWig',
                    sources = srcs,
                    libraries = ["m", "z", "curl", libpython],
                    include_dirs = ['libBigWig'])

setup (name = 'pyBigWig',
       version = '1.0.4',
       description = 'A package for accessing bigWig files using libBigWig',
       author = "Devon P. Ryan",
       author_email = "ryan@ie-freiburg.mpg.de",
       url = "https://github.com/dpryan79/pyBigWig",
       download_url = "https://github.com/dpryan79/pyBigWig/tarball/1.0.1",
       keywords = ["bioinformatics", "bigWig"],
       ext_modules = [module1])
