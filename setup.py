#!/usr/bin/env python
from setuptools import setup, Extension
from distutils import sysconfig
from pathlib import Path
import subprocess
import glob
import sys

srcs = [x for x in 
    glob.glob("libBigWig/*.c")]
srcs.append("pyBigWig.c")

libs=["m", "z"]

# do not link to python on mac, see https://github.com/deeptools/pyBigWig/issues/58
if 'dynamic_lookup' not in (sysconfig.get_config_var('LDSHARED') or ''):
    if sysconfig.get_config_vars('BLDLIBRARY') is not None:
        #Note the "-l" prefix!
        for e in sysconfig.get_config_vars('BLDLIBRARY')[0].split():
            if e[0:2] == "-l":
                libs.append(e[2:])
    elif sys.version_info[0] >= 3 and sys.version_info[1] >= 3:
        libs.append("python%i.%im" % (sys.version_info[0], sys.version_info[1]))
    else:
        libs.append("python%i.%i" % (sys.version_info[0], sys.version_info[1]))

additional_libs = [sysconfig.get_config_var("LIBDIR"), sysconfig.get_config_var("LIBPL")]

defines = []
try:
    foo, _ = subprocess.Popen(['curl-config', '--libs'], stdout=subprocess.PIPE).communicate()
    libs.append("curl")
    foo = foo.decode().strip().split()
except:
    foo = []
    defines.append(('NOCURL', None))
    sys.stderr.write("Either libcurl isn't installed, it didn't come with curl-config, or curl-config isn't in your $PATH. pyBigWig will be installed without support for remote files.\n")

for v in foo:
    if v[0:2] == '-L':
        additional_libs.append(v[2:])

include_dirs = ['libBigWig', sysconfig.get_config_var("INCLUDEPY")]

# Add numpy build information if numpy is installed as a package
try:
    import numpy
    defines.extend([('WITHNUMPY', None), ('NPY_NO_DEPRECATED_API', 'NPY_1_7_API_VERSION')])

    # Ref: https://numpy.org/doc/stable/reference/c-api/coremath.html#linking-against-the-core-math-library-in-an-extension
    numpy_include_dir = numpy.get_include()
    numpy_library_dir = str(Path(numpy_include_dir) / '..' / 'lib')

    include_dirs.append(numpy_include_dir)
    additional_libs.append(numpy_library_dir)
    libs.append('npymath')
# Silently ignore a failed import of numpy
except ImportError:
    pass

module1 = Extension('pyBigWig',
                    sources = srcs,
                    libraries = libs,
                    library_dirs = additional_libs, 
                    define_macros = defines,
                    include_dirs = include_dirs)

setup(
    ext_modules=[module1]
)
