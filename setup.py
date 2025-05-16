import os
import platform
import subprocess
import sys
import shutil
import glob
from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext

NAME = "qlib"
VERSION = "1.0"
DESCRIPTION = ""
AUTHOR = "qinyonghang"
AUTHOR_EMAIL = "yonghang.qin@google.com"
URL = ""
INSTALL_REQUIRES = []


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = [
            "-DPython3_EXECUTABLE=" + sys.executable,
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_PYTHON_MODULE=ON",  # Custom flag to indicate building for Python
        ]

        build_args = ["--config", "Release"]
        if platform.system() == "Windows":
            cmake_args += [
                "-A",
                "x64" if platform.architecture()[0] == "64bit" else "Win32",
            ]
            build_args += ["--parallel 4"]
        else:
            build_args += ["--", "-j4"]
        build_folder = os.path.abspath(self.build_temp)

        if not os.path.exists(build_folder):
            os.makedirs(build_folder)

        cmake_setup = ["cmake", ext.sourcedir] + cmake_args
        cmake_build = ["cmake", "--build", "."] + build_args

        print("Building extension for Python {}".format(sys.version.split("\n", 1)[0]))
        print("Invoking CMake setup: '{}'".format(" ".join(cmake_setup)))
        sys.stdout.flush()
        subprocess.check_call(cmake_setup, cwd=build_folder)
        print("Invoking CMake build: '{}'".format(" ".join(cmake_build)))
        sys.stdout.flush()
        subprocess.check_call(cmake_build, cwd=build_folder)

        os.makedirs(extdir, exist_ok=True)
        if platform.system() == "Windows":
            pyd_file = os.path.join(build_folder, "Release", f"{NAME}_python.pyd")
            dst_file = os.path.join(extdir, f"{NAME}.pyd")
        else:
            pyd_file = os.path.join(build_folder, f"{NAME}_python.so")
            dst_file = os.path.join(extdir, f"{NAME}.so")
        shutil.copy(pyd_file, dst_file)
        print(f"Copying {pyd_file} to ext directory: {extdir}")

        if platform.system() == "Windows":
            other_libs = glob.glob(os.path.join(build_folder, "*.dll"))
        else:
            other_libs = glob.glob(os.path.join(build_folder, "*.so"))
        for lib in other_libs:
            if lib == pyd_file:
                continue
            shutil.copy(lib, extdir)
            print(f"Copying {lib} to ext directory: {extdir}")


setup(
    name=NAME,
    version=VERSION,
    description=DESCRIPTION,
    author=AUTHOR,
    author_email=AUTHOR_EMAIL,
    license="Apache License 2.0",
    url=URL,
    install_requires=INSTALL_REQUIRES,
    ext_modules=[CMakeExtension(NAME, ".")],  # Adjust the source directory
    cmdclass=dict(build_ext=CMakeBuild),
    packages=find_packages(where="package"),
    package_data={"": ["*.pyd", "*.so", "*.dll"]},
    include_package_data=True,
    package_dir={"": "package"},
    keywords=["viewer", "MMD", "PMD", "PMX", "OBJ"],
)
