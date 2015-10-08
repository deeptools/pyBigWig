from distutils.core import setup, Extension
import subprocess
import glob

srcs = [x for x in 
    glob.glob("libBigWig/*.c")]
srcs.append("pyBigWig.c")

module1 = Extension('pyBigWig',
                    sources = srcs,
                    libraries = ["m", "z", "curl"],
                    lib_dirs = ["/usr/lib/x86_64-linux-gnu"],
                    include_dirs = ['libBigWig'])

setup (name = 'pyBigWig',
       version = '1.0',
       description = 'A package for accessing bigWig files',
       ext_modules = [module1])
