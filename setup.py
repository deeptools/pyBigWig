#!/usr/bin/env python
from setuptools import setup, Extension, find_packages
from distutils import sysconfig
import subprocess
import glob
import sys

srcs = [x for x in 
    glob.glob("libBigWig/*.c")]
srcs.append("pyBigWig.c")

libs=["m", "z", "curl"]
if sysconfig.get_config_vars('BLDLIBRARY') is not None:
    #Note the "-l" prefix!
    for e in sysconfig.get_config_vars('BLDLIBRARY')[0].split():
        if e[0:2] == "-l":
            libs.append(e[2:])
elif(sys.version_info[0] >= 3 and sys.version_info[1] >= 3) :
    libs.append("python%i.%im" % (sys.version_info[0], sys.version_info[1]))
else :
    libs.append("python%i.%i" % (sys.version_info[0], sys.version_info[1]))

additional_libs = [sysconfig.get_config_var("LIBDIR"), sysconfig.get_config_var("LIBPL")]
foo, _ = subprocess.Popen(['curl-config', '--libs'], stdout=subprocess.PIPE).communicate()
foo = foo.strip().split()
for v in foo:
    if(v[0:2] == "-L") :
        additional_libs.append(v[2:])

module1 = Extension('pyBigWig',
                    sources = srcs,
                    libraries = libs,
                    library_dirs = additional_libs, 
                    include_dirs = ['libBigWig', sysconfig.get_config_var("INCLUDEPY")])

setup(name = 'pyBigWig',
       version = '0.2.0',
       description = 'A package for accessing bigWig files using libBigWig',
       author = "Devon P. Ryan",
       author_email = "ryan@ie-freiburg.mpg.de",
       url = "https://github.com/dpryan79/pyBigWig",
       download_url = "https://github.com/dpryan79/pyBigWig/tarball/0.2.0",
       keywords = ["bioinformatics", "bigWig"],
       packages = find_packages(),
       include_package_data=True,
       ext_modules = [module1])
