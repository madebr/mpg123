#!/usr/bin/env python3

import argparse
import functools
import os
import pathlib
import shutil
import subprocess

GIT_MASTER = "c2884b774ad46852ff9038bbed80fd6ba19597de"
GIT_INTEL = "assembly-intel-syntax"

def find_objects(tree):
    result = []
    for root, _, files in os.walk(tree):
        for f in files:
            if f.endswith(".S.obj") or f.endswith(".S.o"):
                result.append(pathlib.Path(root) / f)
    return result

def build_directory_for_id(root: Pathlib.Path, dirid: str):
    return root / f"build-{dirid}"

def configure_build(root: pathlib.Path, dirid: str, fast: bool, cmake_flags: list[str]):
    build_dir = build_directory_for_id(root=root, dirid=dirid)
    if not fast:
        shutil.rmtree(build_dir, ignore_errors=True)
    try:
        original_objects = find_objects(build_dir)
    except (FileNotFoundError, IOError):
        original_objects = []
    for obj in original_objects:
        os.unlink(obj)

    print(f"Build {dirid} objects")
    cmake_cmd = ["cmake", "-S", root / "ports/cmake", "-B", build_dir, "-GNinja"] + cmake_flags
    if not fast:
        cmake_cmd += ["--fresh"]
    subprocess.check_call(cmake_cmd)
    subprocess.check_call(["cmake", "--build", build_dir, "--target", "libmpg123", "--config", "Release"])

def build_unix(root: pathlib.Path, fast: bool):
    subprocess.check_call(["git", "-C", str(root), "checkout", GIT_MASTER])
    configure_build(root=root, fast=fast, dirid="original", cmake_flags=["-DUSE_ASSEMBLER=TRUE", "-DUSE_NASM=FALSE", "-DCMAKE_BUILD_TYPE=Release"])
    configure_build(root=root, fast=fast, dirid="original32", cmake_flags=["-DUSE_ASSEMBLER=TRUE", "-DUSE_NASM=FALSE", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_FLAGS=-m32", "-DCMAKE_ASM_FLAGS=-m32"])

    subprocess.check_call(["git", "-C", str(root), "checkout", GIT_INTEL])
    configure_build(root=root, fast=fast, dirid="intel", cmake_flags=["-DUSE_ASSEMBLER=TRUE", "-DUSE_NASM=FALSE", "-DCMAKE_BUILD_TYPE=Release"])
    configure_build(root=root, fast=fast, dirid="intel32", cmake_flags=["-DUSE_ASSEMBLER=TRUE", "-DUSE_NASM=FALSE", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_FLAGS=-m32", "-DCMAKE_ASM_FLAGS=-m32"])

    print("Done")

def build_msvc(root: pathlib.Path, fast: bool, generator: str):
    subprocess.check_call(["git", "-C", str(root), "checkout", GIT_MASTER])
    yasm = shutil.which("yasm")
    assert yasm, "Could not find yasm"
    configure_build(root=root, fast=fast, dirid="original", cmake_flags=[f"-DYASM_ASSEMBLER={yasm}", "-G", generator, "-A", "x64"])
    configure_build(root=root, fast=fast, dirid="original32", cmake_flags=[f"-DYASM_ASSEMBLER={yasm}", "-G", generator, "-A", "Win32"])

    subprocess.check_call(["git", "-C", str(root), "checkout", GIT_INTEL])
    configure_build(root=root, fast=fast, dirid="intel", cmake_flags=["-DUSE_ASSEMBLER=TRUE", "-DUSE_NASM=FALSE", "-G", generator, "-A", "x64"])
    configure_build(root=root, fast=fast, dirid="intel32", cmake_flags=["-DUSE_ASSEMBLER=TRUE", "-DUSE_NASM=FALSE", "-G", generator, "-A", "Win32"])

    print("Done")

def disassemble(dirid: str, root: pathlib.Path, compare_dir: pathlib.Path, disassemble_callback: typing.Callback[[pathlib.Path], str]):
    object_paths = find_objects(build_directory_for_id(root=root, dirid=dirid))
    d = compare_dir / dirid
    d.mkdir(parents=True)
    paths = []
    for obj_ath in object_paths:
        disasm_path = (d / obj_ath.name).with_suffix(".txt")
        paths.append(disasm_path)
        disasm = disassemble_callback(obj_ath)
        disasm_path.write_text(disasm)
    return paths

def compare_dirids(disassemble_callback: typing.Callback[[pathlib.Path], str], compare_dir: pathlib.Path, root: pathlib.Path, dirid_original: str, dirid_intel: str):

    original_paths = disassemble(compare_dir=compare_dir, root=root, disassemble_callback=disassemble_callback, dirid=dirid_original)
    intel_paths = disassemble(compare_dir=compare_dir, root=root, disassemble_callback=disassemble_callback, dirid=dirid_intel)
    assert len(original_paths) > 0, f"Number of objects in {compare_dir} must be >0"
    assert len(original_paths) == len(intel_paths), f"Number of objects of {dirid_original} ({len(original_paths)}) must be equal to number of objects of {dirid_intel} ({len(intel_paths)})"

    print(f"== Comparing {dirid_original} and {dirid_intel} ==")
    for original_path in original_paths:
        intel_path_name = original_path.name.replace(".S", "-intel.S")
        intel_path = compare_dir / dirid_intel / intel_path_name
        assert intel_path.is_file()
        diff_path = compare_dir / f"diff-{original_path.name}.diff"
        proc = subprocess.run(["diff", "-u", original_path, intel_path], stdout=subprocess.PIPE, check=False, text=True)
        diff_path.write_text(proc.stdout)
        if proc.returncode == 0:
            print(f"OK   {original_path.name}")
        else:
            print(f"FAIL {original_path.name} ({diff_path})")
    print()


def disassemble_objdump(obj_path: pathlib.Path) -> str:
    disasm = subprocess.check_output(["objdump", "-dr", str(obj_path)], text=True)
    disasm = "\n".join(line for line in disasm.splitlines() if "file format" not in line)
    return disasm


def disassemble_dumpbin(obj_path: pathlib.Path, dumpbin: pathlib.Path) -> str:
    disasm = subprocess.check_output([str(dumpbin), "/disasm", str(obj_path)], text=True)
    return disasm


def compare_unix(root: pathlib.Path):
    compare_dir = root / "compare"
    shutil.rmtree(compare_dir, ignore_errors=True)

    compare_dirids(compare_dir=compare_dir, root=root, dirid_original="original",   disassemble_callback=disassemble_objdump, dirid_intel="intel")
    compare_dirids(compare_dir=compare_dir, root=root, dirid_original="original32", disassemble_callback=disassemble_objdump, dirid_intel="intel32")

def compare_msvc(dumpbin: pathlib.Path, root: pathlib.Path):
    compare_dir = root / "compare"
    shutil.rmtree(compare_dir, ignore_errors=True)

    disassemble_callback = functools.partial(disassemble_dumpbin, dumpbin=dumpbin)

    compare_dirids(compare_dir=compare_dir, root=root, dirid_original="original",   disassemble_callback=disassemble_callback, dirid_intel="intel")
    compare_dirids(compare_dir=compare_dir, root=root, dirid_original="original32", disassemble_callback=disassemble_callback, dirid_intel="intel32")

def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.set_defaults(action=None)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path().resolve())

    subparsers = parser.add_subparsers()

    parser_build_unix = subparsers.add_parser("build-unix")
    parser_build_unix.set_defaults(action="build_unix")
    parser_build_unix.add_argument("--fast", action="store_true")

    parser_compare_unix = subparsers.add_parser("compare-unix")
    parser_compare_unix.set_defaults(action="compare_unix")

    parser_build_msvc = subparsers.add_parser("build-msvc")
    parser_build_msvc.set_defaults(action="build_msvc")
    parser_build_msvc.add_argument("--fast", action="store_true")
    parser_build_msvc.add_argument(
        "--generator",
        choices=("Visual Studio 15 2017", "Visual Studio 16 2019", "Visual Studio 17 2022", "Visual Studio 18 2016"),
        default="Visual Studio 16 2019",
    )

    parser_compare_msvc = subparsers.add_parser("compare-msvc")
    parser_compare_msvc.set_defaults(action="compare_msvc")
    parser_compare_msvc.add_argument("--dumpbin", type=pathlib.Path, default=shutil.which("dumpbin"))

    args = parser.parse_args()

    match args.action:
        case "build_unix":
            build_unix(root=args.root, fast=args.fast)
        case "compare_unix":
            compare_unix(root=args.root)
        case "build_msvc":
            build_msvc(root=args.root, fast=args.fast, generator=args.generator)
        case "compare_msvc":
            if not args.dumpbin:
                parser.error("--dumpbin is required")
            if not args.dumpbin.is_file():
                parser.error("Invalid dumpbin path")
            compare_msvc(root=args.root, dumpbin=args.dumpbin)
        case _:
            parser.error("Action required")

if __name__ == "__main__":
    raise SystemExit(main())
