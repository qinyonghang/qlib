import argparse
import os
import platform
# import sys

# sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from scripts.downloader import Downloader


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("name", type=str)
    parser.add_argument("url", type=str)
    parser.add_argument("--url_hash", type=str, default=None)
    parser.add_argument("--download_dir", type=str)
    parser.add_argument("--source_dir", type=str)
    parser.add_argument("--build_dir", type=str, default=None)
    parser.add_argument("--install_dir", type=str, default=None)
    parser.add_argument("--build_type", type=str, default="release")
    parser.add_argument("--cmakelists_dir", type=str, default=None)
    parser.add_argument("--cmake_args", type=str, default=None)
    parser.add_argument("--custom_compile", nargs="+", default=None)
    parser.add_argument("--skip_compile", action="store_true")
    parser.add_argument("--skip_install", action="store_true")
    parser.add_argument("--before_compile", nargs="+", default=None)
    parser.add_argument("--after_compile", nargs="+", default=None)
    parser.add_argument("--working_thread", type=int, default=4)
    parser.add_argument("--system", type=str, default=platform.system())
    parser.add_argument("--arch", type=str, default="64")
    return parser.parse_args()


if __name__ == "__main__":
    args = get_args()

    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

    if args.download_dir is None:
        args.download_dir = os.path.join(root_dir, "third_party")

    if args.source_dir is None:
        args.source_dir = os.path.join(args.download_dir, args.name)

    if args.system == "Windows":
        suffix = "windows" if args.arch == "64" else "win32"
    elif args.system == "Linux":
        suffix = "linux" if args.arch == "64" else "linux32"
    else:
        suffix = ""

    if args.build_dir is None:
        args.build_dir = os.path.join(args.source_dir, "build2", suffix)

    if args.install_dir is None:
        args.install_dir = os.path.join(args.source_dir, "install2", suffix)

    if args.cmakelists_dir is None:
        args.cmakelists_dir = args.source_dir

    if os.path.exists(args.install_dir):
        print("Already compiled!")
        exit(0)

    if args.skip_install and os.path.exists(args.build_dir):
        exit(0)

    print(f"Starting download for {args.url}...")
    downloader = Downloader(args.download_dir)
    ok = downloader(args.name, args.url, args.url_hash, args.source_dir)
    if not ok:
        print("Download failed!")
        exit(1)
    print("Download complete!")

    if args.skip_compile:
        exit(0)

    os.chdir(args.source_dir)

    if args.before_compile is not None:
        for cmd in args.before_compile:
            os.system(cmd)

    if args.custom_compile is not None:
        for cmds in args.custom_compile:
            cmds = cmds.split(";")
            for cmd in cmds:
                print(f"Command: {cmd}")
                os.system(cmd)
    elif not (os.path.exists(args.build_dir) and os.path.exists(args.install_dir)):
        build_type = args.build_type.capitalize()
        configure_command = f"cmake -S {args.cmakelists_dir} -B {args.build_dir} -DCMAKE_INSTALL_PREFIX={args.install_dir} -DCMAKE_BUILD_TYPE={build_type}"
        if args.cmake_args is not None:
            configure_command += f" {args.cmake_args}"
        os.system(configure_command)

        if platform.system() == "Windows":
            os.system(
                f"cmake --build {args.build_dir} --config {build_type} --target ALL_BUILD --parallel {args.working_thread}"
            )
        elif platform.system() == "Linux":
            os.system(f"cmake --build {args.build_dir} -j{args.working_thread}")

        if not args.skip_install:
            if platform.system() == "Windows":
                os.system(
                    f"cmake --build {args.build_dir} --config {build_type} --target INSTALL"
                )
            elif platform.system() == "Linux":
                os.system(f"cmake --install {args.build_dir}")

    if args.after_compile is not None:
        for cmd in args.after_compile:
            os.system(cmd)
