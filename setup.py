from distutils.core import setup, Extension
import subprocess
import platform
import glob

srcs = [x for x in 
    glob.glob("kent/src/lib/*.c")]
srcs.append("pyBigWig.c")

module1 = Extension('pyBigWig',
                    sources = srcs,
                    libraries = ["m", "z", "ssl"],
                    define_macros = [("_FILE_OFFSET_BITS",64),
                                     ("_LARGEFILE_SOURCE",None),
                                     ("_GNU_SOURCE",None),
                                     ("MACHTYPE_x86_64",None),
                                     ("USE_SSL",None)],
                    include_dirs = ['kent/src/inc'])

setup (name = 'pyBigWig',
       version = '1.0',
       description = 'A package for accessing bigWig files',
       ext_modules = [module1])
