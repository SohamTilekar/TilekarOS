#!/usr/bin/env python
# Should suport 2.7, 3.0-3.6, 3.7-3.9, 3.10+ version
# maintain 100% backward compatibility
from __future__ import print_function, unicode_literals

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

try:
    from typing import List, Optional, Tuple, Union
except ImportError:
    # Python 2.7 doesn't have typing, but we can define dummy types
    List = list
    Optional = None
    Tuple = tuple
    Union = None


ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
SYSROOT = ROOT / "sysroot"


def _read_text(path_obj):
    """Read file content with Python 2.7 compatibility."""
    with open(str(path_obj), "rb") as f:
        content = f.read()
    if isinstance(content, bytes):
        return content.decode("utf-8", errors="ignore")
    return content


def _write_text(path_obj, content):
    """Write file content with Python 2.7 compatibility."""
    with open(str(path_obj), "wb") as f:
        if isinstance(content, bytes):
            f.write(content)
        else:
            f.write(content.encode("utf-8"))


USE_COLOR = sys.stdout.isatty()
RESET = "\033[0m"
BOLD = "\033[1m"
DIM = "\033[2m"
RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
MAGENTA = "\033[35m"
CYAN = "\033[36m"


def _style(text, *codes):
    if not USE_COLOR:
        return text
    return "".join(codes) + text + RESET


def banner(title):
    line = "═" * 68
    print(_style("\n{0}".format(line), CYAN))
    print(_style("🚀 {0}".format(title), BOLD, CYAN))
    print(_style(line, CYAN))


def step(message):
    print("{0} {1}".format(_style("🔹", BLUE), _style(message, BOLD)))


def info(message):
    print("{0} {1}".format(_style("ℹ️ ", CYAN), message))


def ok(message):
    print("{0} {1}".format(_style("✅", GREEN), _style(message, GREEN)))


def warn(message):
    print("{0} {1}".format(_style("⚠️ ", YELLOW), _style(message, YELLOW)))


def fail(message):
    print("{0} {1}".format(_style("❌", RED), _style(message, RED)))


def run(cmd, cwd=None):
    print(_style("   $ {0}".format(" ".join(cmd)), DIM))
    subprocess.check_call(cmd, cwd=cwd or str(ROOT))


def run_allow_failure(cmd, cwd=None):
    print(_style("   $ {0}".format(" ".join(cmd)), DIM))
    proc = subprocess.Popen(
        cmd, cwd=cwd or str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    proc.communicate()
    return proc.returncode


def first_drive_name(drives):
    return parse_drives(drives)[0].name


class Drive(object):
    def __init__(self, name, size_mb, interface):
        self.name = name
        self.size_mb = size_mb
        self.interface = interface

    def __repr__(self):
        return "Drive(name={0!r}, size_mb={1!r}, interface={2!r})".format(
            self.name, self.size_mb, self.interface
        )


def parse_drives(drives):
    out = []
    for raw in [x.strip() for x in drives.split(",") if x.strip()]:
        parts = raw.split(":")
        name = parts[0]
        size_mb = 24
        interface = "ide"
        if len(parts) >= 2 and parts[1]:
            size_mb = int(parts[1])
        if len(parts) >= 3 and parts[2]:
            interface = parts[2].lower()
        if interface not in {"ide", "ahci"}:
            raise ValueError(
                "Unsupported attach_type '{0}' for drive '{1}'".format(interface, name)
            )
        out.append(Drive(name=name, size_mb=size_mb, interface=interface))
    if not out:
        out.append(Drive(name="boot", size_mb=24, interface="ide"))
    return out


def configure(arch, enable_test=False):
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    cmake_cache = BUILD_DIR / "CMakeCache.txt"
    cmake_cmd = [
        "cmake",
        "-S",
        str(ROOT),
        "-B",
        str(BUILD_DIR),
        "-DOS_ARCH={0}".format(arch),
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_ASM_NASM_COMPILER=nasm",
    ]

    if enable_test:
        cmake_cmd.append("-DENABLE_TEST=1")

    toolchain = ROOT / "cmake" / "toolchains" / "{0}.cmake".format(arch)
    if cmake_cache.exists():
        text = _read_text(cmake_cache)
        if "CMAKE_C_COMPILER:FILEPATH=/usr/bin/cc" in text:
            shutil.rmtree(str(BUILD_DIR), ignore_errors=True)
            BUILD_DIR.mkdir(parents=True, exist_ok=True)
            cmake_cache = BUILD_DIR / "CMakeCache.txt"
    if toolchain.exists():
        cmake_cmd.append("-DCMAKE_TOOLCHAIN_FILE={0}".format(toolchain))
    elif cmake_cache.exists():
        text = _read_text(cmake_cache)
        if "CMAKE_TOOLCHAIN_FILE" in text:
            cmake_cache.unlink()

    if cmake_cache.exists():
        cache_text = _read_text(cmake_cache)
        arch_ok = (
            "OS_ARCH:UNINITIALIZED={0}".format(arch) in cache_text
            or "OS_ARCH:STRING={0}".format(arch) in cache_text
        )
        cc_ok = (
            "CMAKE_C_COMPILER:UNINITIALIZED=clang" in cache_text
            or "CMAKE_C_COMPILER:FILEPATH=/usr/bin/clang" in cache_text
        )
        nasm_ok = (
            "CMAKE_ASM_NASM_COMPILER:UNINITIALIZED=nasm" in cache_text
            or "CMAKE_ASM_NASM_COMPILER:FILEPATH=/usr/bin/nasm" in cache_text
        )
        if arch_ok and cc_ok and nasm_ok:
            step("Using existing CMake configuration (arch={0})".format(arch))
            return

    step("Configuring CMake (arch={0})".format(arch))
    run(cmake_cmd)


def build_target(target):
    step("Building target: {0}".format(target))
    run(["cmake", "--build", str(BUILD_DIR), "--target", target])


def sync_sysroot_headers():
    step("Syncing sysroot headers (libc + kernel)")
    include_dir = SYSROOT / "usr" / "include"
    include_dir.mkdir(parents=True, exist_ok=True)
    run(
        [
            "cmake",
            "-E",
            "copy_directory",
            str(ROOT / "libc" / "include"),
            str(include_dir),
        ]
    )
    run(
        [
            "cmake",
            "-E",
            "copy_directory",
            str(ROOT / "kernel" / "include"),
            str(include_dir),
        ]
    )


def build_userland(arch):
    banner("TilekarOS Userland Build")
    userland_dir = ROOT / "userland"
    bin_dir = SYSROOT / "bin"
    bin_dir.mkdir(parents=True, exist_ok=True)
    if not userland_dir.exists():
        warn("Userland directory {0} not found".format(userland_dir))
        return

    for program_dir in userland_dir.iterdir():
        if program_dir.is_dir():
            main_c = program_dir / "main.c"
            if main_c.exists():
                prog_name = "sh" if program_dir.name == "shell" else program_dir.name
                out_file = bin_dir / prog_name
                compile_user_program(arch, str(main_c), str(out_file))


def ensure_sysroot(arch, do_configure=True):
    step("Preparing sysroot")
    if do_configure:
        configure(arch)
    build_target("libc")
    build_target("sysroot_extras")
    sync_sysroot_headers()
    build_userland(arch)


def collect_workspace_c_files(vm_dir):
    return sorted(vm_dir.glob("*.c"))


def create_empty_fat_image(img, size_mb):
    step("Creating empty FAT image: {0} ({1}MB)".format(img.name, size_mb))
    run(
        [
            "dd",
            "if=/dev/zero",
            "of={0}".format(img),
            "bs=1M",
            "count={0}".format(size_mb),
            "status=none",
        ]
    )
    run(["mkfs.fat", str(img)])


def create_image_from_export(img, size_mb, export_dir):
    step("Recreating {0} from exported content: {1}".format(img.name, export_dir))
    create_empty_fat_image(img, size_mb)
    if export_dir.exists():
        items = [str(p) for p in export_dir.iterdir()]
        if items:
            run(["mcopy", "-i", str(img), "-snD", "o", "-s"] + items + ["::/"])


def is_fat_image_healthy(img):
    return run_allow_failure(["minfo", "-i", str(img), "::"]) == 0


def write_drive_meta(meta_path, size_mb):
    _write_text(meta_path, str(size_mb))


def read_drive_meta(meta_path):
    if not meta_path.exists():
        return None
    try:
        return int(_read_text(meta_path).strip())
    except ValueError:
        return None


def ensure_vm_workspace(vm, drives_cfg):
    step("Preparing VM workspace: {0}".format(vm))
    vm_dir = ROOT / vm
    drives_dir = vm_dir / "drives"
    export_root = vm_dir / "exported_drives"
    drives_dir.mkdir(parents=True, exist_ok=True)
    export_root.mkdir(parents=True, exist_ok=True)

    drives = parse_drives(drives_cfg)
    step("Drive configuration: {0}".format(drives_cfg))
    for d in drives:
        img = drives_dir / "{0}.img".format(d.name)
        meta = drives_dir / "{0}.meta".format(d.name)
        export_dir = export_root / d.name

        if export_dir.exists() and any(export_dir.iterdir()):
            create_image_from_export(img, d.size_mb, export_dir)
            write_drive_meta(meta, d.size_mb)
            continue

        old_size = read_drive_meta(meta)
        recreate = (not img.exists()) or (old_size != d.size_mb)
        if img.exists() and not recreate and not is_fat_image_healthy(img):
            warn("Detected FAT corruption in {0}; recreating image".format(img.name))
            recreate = True
        if recreate:
            create_empty_fat_image(img, d.size_mb)
            write_drive_meta(meta, d.size_mb)
    return vm_dir, drives


def inject_boot_payload(vm_dir, boot_img, kernel_binary):
    step(
        "Injecting kernel, sysroot, and VM C files into drive: {0}".format(
            boot_img.name
        )
    )
    run_allow_failure(["mmd", "-i", str(boot_img), "-D", "o", "::/boot"])
    run(
        [
            "mcopy",
            "-i",
            str(boot_img),
            "-D",
            "o",
            str(kernel_binary),
            "::/boot/myos.kernel",
        ]
    )

    if SYSROOT.exists():
        step("Merging sysroot into boot drive")
        items = [str(p) for p in SYSROOT.iterdir()]
        if items:
            run(["mcopy", "-i", str(boot_img), "-snD", "o", "-s"] + items + ["::/"])

    for cfile in collect_workspace_c_files(vm_dir):
        run(
            [
                "mcopy",
                "-i",
                str(boot_img),
                "-D",
                "o",
                str(cfile),
                "::/{0}".format(cfile.stem),
            ]
        )

    test_bin_dir = ROOT / vm_dir.name / "exported_drives" / "boot" / "BIN"
    if test_bin_dir.exists():
        step("Merging test binaries into boot drive")
        run(["mcopy", "-i", str(boot_img), "-snD", "o", "-s", str(test_bin_dir), "::/"])


def export_drives(vm_dir, drives):
    step("Exporting drives to exported_drives")
    drives_dir = vm_dir / "drives"
    export_root = vm_dir / "exported_drives"
    export_root.mkdir(parents=True, exist_ok=True)
    for d in drives:
        img = drives_dir / "{0}.img".format(d.name)
        if not img.exists():
            continue
        export_dir = export_root / d.name
        export_dir.mkdir(parents=True, exist_ok=True)
        attempts = [
            ["mcopy", "-i", str(img), "-snD", "o", "::/*", "{0}/".format(export_dir)],
            [
                "mcopy",
                "-i",
                "{0}@@512".format(img),
                "-snD",
                "o",
                "::/*",
                "{0}/".format(export_dir),
            ],
        ]
        exported = False
        for cmd in attempts:
            print(_style("   $ {0}".format(" ".join(cmd)), DIM))
            proc = subprocess.Popen(
                cmd, cwd=str(ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
            proc.communicate()
            if proc.returncode == 0:
                exported = True
                break
        if not exported:
            warn("Skipping export for {0}.img (FAT not readable)".format(d.name))


def qemu_drive_flags(vm_dir, drives):
    flags = []
    has_ahci = False
    ahci_num = 0
    ide_num = 0
    for _, d in enumerate(drives):
        boot_prop = ",bootindex=0" if (ide_num + ahci_num) == 0 else ""
        img = vm_dir / "drives" / "{0}.img".format(d.name)
        if d.interface == "ahci":
            if not has_ahci:
                flags += ["-device", "ich9-ahci,id=ahci0"]
                has_ahci = True
            flags += [
                "-drive",
                "file={0},format=raw,if=none,id=ahci_d{1}".format(img, ahci_num),
            ]
            flags += [
                "-device",
                "ide-hd,bus=ahci0.{0},drive=ahci_d{1}{2}".format(
                    ahci_num, ahci_num, boot_prop
                ),
            ]
            ahci_num += 1
            continue
        bus = ide_num // 2
        unit = ide_num % 2
        if ide_num < 4:
            flags += [
                "-drive",
                "file={0},format=raw,if=ide,bus={1},unit={2}".format(img, bus, unit),
            ]
        else:
            extra_idx = ((ide_num - 4) // 4) + 1
            extra_drive_idx = (ide_num - 4) % 4
            extra_bus = extra_drive_idx // 2
            extra_unit = extra_drive_idx % 2
            if extra_drive_idx == 0:
                flags += ["-device", "piix4-ide,id=extide{0}".format(extra_idx)]
            flags += [
                "-drive",
                "file={0},format=raw,if=none,id=d{1}".format(img, ide_num),
            ]
            flags += [
                "-device",
                "ide-hd,bus=extide{0}.{1},drive=d{2},unit={3}{4}".format(
                    extra_idx, extra_bus, ide_num, extra_unit, boot_prop
                ),
            ]
        ide_num += 1
    return flags


def run_qemu(arch, vm, drives_cfg, mode, enable_test=False):
    banner("TilekarOS Run Pipeline ({0})".format(mode))
    step("Run mode: {0}".format(mode))
    vm_dir, drives = ensure_vm_workspace(vm, drives_cfg)
    configure(arch, enable_test=enable_test)
    build_target("myos.kernel")
    ensure_sysroot(arch, do_configure=False)

    if enable_test:
        compile_test_programs(arch, vm)

    kernel_binary = BUILD_DIR / "kernel" / "myos.kernel"
    boot_img = vm_dir / "drives" / "{0}.img".format(first_drive_name(drives_cfg))

    qemu_cmd = ["qemu-system-{0}".format(arch)]
    if mode == "run":
        qemu_cmd += ["-kernel", str(kernel_binary)]
        if SYSROOT.exists():
            step("Merging sysroot into boot drive")
            items = [str(p) for p in SYSROOT.iterdir()]
            if items:
                run(["mcopy", "-i", str(boot_img), "-snD", "o", "-s"] + items + ["::/"])

        test_bin_dir = ROOT / vm / "exported_drives" / "boot" / "BIN"
        if enable_test and test_bin_dir.exists():
            step("Merging test binaries into boot drive")
            run(
                [
                    "mcopy",
                    "-i",
                    str(boot_img),
                    "-snD",
                    "o",
                    "-s",
                    str(test_bin_dir),
                    "::/",
                ]
            )
    elif mode == "run_iso":
        build_target("iso")
        qemu_cmd += ["-boot", "d", "-cdrom", str(BUILD_DIR / "myos.iso")]
        if SYSROOT.exists():
            step("Merging sysroot into boot drive")
            items = [str(p) for p in SYSROOT.iterdir()]
            if items:
                run(["mcopy", "-i", str(boot_img), "-snD", "o", "-s"] + items + ["::/"])

        test_bin_dir = ROOT / vm / "exported_drives" / "boot" / "BIN"
        if enable_test and test_bin_dir.exists():
            step("Merging test binaries into boot drive")
            run(
                [
                    "mcopy",
                    "-i",
                    str(boot_img),
                    "-snD",
                    "o",
                    "-s",
                    str(test_bin_dir),
                    "::/",
                ]
            )
    elif mode == "run_disk":
        inject_boot_payload(vm_dir, boot_img, kernel_binary)
        isodir = BUILD_DIR / "disk_isodir"
        grub_iso = BUILD_DIR / "disk_boot.iso"
        shutil.rmtree(str(isodir), ignore_errors=True)
        run(["cmake", "-E", "make_directory", str(isodir / "boot" / "grub")])
        run(
            [
                "cmake",
                "-E",
                "copy",
                str(kernel_binary),
                str(isodir / "boot" / "myos.kernel"),
            ]
        )
        run(
            [
                "cmake",
                "-E",
                "copy",
                str(ROOT / "grub.cfg"),
                str(isodir / "boot" / "grub" / "grub.cfg"),
            ]
        )

        if SYSROOT.exists():
            step("Merging sysroot into disk image")
            run(["cmake", "-E", "copy_directory", str(SYSROOT), str(isodir)])

        run(["grub-mkrescue", "-o", str(grub_iso), str(isodir)])
        qemu_cmd += ["-boot", "d", "-cdrom", str(grub_iso)]
    else:
        raise ValueError("Unsupported run mode {0}".format(mode))

    qemu_cmd += qemu_drive_flags(vm_dir, drives)
    qemu_cmd += ["-d", "guest_errors,unimp", "-D", str(vm_dir / "qemu.log")]
    qemu_cmd += ["-serial", "file:{0}".format(vm_dir / "serial.log")]
    qemu_cmd += ["-monitor", "stdio"]
    step("Launching QEMU")
    info("QEMU command:")
    print(_style("  " + " ".join(qemu_cmd), MAGENTA))
    run(qemu_cmd)
    export_drives(vm_dir, drives)
    ok("Run pipeline completed")


def compile_user_program(arch, src_file, out_file=None):
    if not src_file:
        raise ValueError("comp requires --file <path-to-c-file>")
    src = Path(src_file)
    out = Path(out_file) if out_file else src.with_suffix("")
    out.parent.mkdir(parents=True, exist_ok=True)
    step("Compiling userspace program: {0} -> {1}".format(src, out))
    run(
        [
            "clang",
            "--target={0}-elf".format(arch),
            "--sysroot={0}".format(SYSROOT),
            "-nostdlib",
            "-ffreestanding",
            "-fno-pic",
            "-fno-pie",
            "-static",
            "-O2",
            "-Wall",
            "-Wl,-z,noexecstack",
            "-Wl,--build-id=none",
            "-T",
            str(SYSROOT / "usr" / "lib" / "user.ld"),
            str(SYSROOT / "usr" / "lib" / "crt0.o"),
            str(src),
            str(SYSROOT / "usr" / "lib" / "libc.a"),
            "-o",
            str(out),
        ]
    )
    ok("Built userspace executable: {0}".format(out))


def compile_test_programs(arch, vm):
    banner("TilekarOS Test Programs Compilation")
    boot_dir = ROOT / vm / "exported_drives" / "boot"
    if not boot_dir.exists():
        warn("Boot directory {0} not found, skipping test programs".format(boot_dir))
        return

    bin_dir = boot_dir / "BIN"
    bin_dir.mkdir(parents=True, exist_ok=True)

    for c_file in boot_dir.glob("*.c"):
        out_name = c_file.stem.upper()
        out_file = bin_dir / out_name
        compile_user_program(arch, str(c_file), str(out_file))


def main():
    parser = argparse.ArgumentParser(description="TilekarOS Python build orchestrator")
    parser.add_argument(
        "command",
        choices=[
            "configure",
            "kernel",
            "sysroot",
            "userland",
            "iso",
            "run",
            "run_iso",
            "run_disk",
            "export_drives",
            "comp",
            "clean",
        ],
    )
    parser.add_argument("--arch", default=os.environ.get("ARCH", "i386"))
    parser.add_argument("--vm", default=os.environ.get("VM", "VirtualMachine"))
    parser.add_argument("--drives", default=os.environ.get("DRIVES", "boot:24:ide"))
    parser.add_argument("--file", default=os.environ.get("FILE"))
    parser.add_argument("--out", default=os.environ.get("OUT"))
    parser.add_argument(
        "--enable-test", action="store_true", help="Enable automatic test execution"
    )
    args = parser.parse_args()

    try:
        if args.command == "configure":
            banner("TilekarOS Configure")
            configure(args.arch, enable_test=args.enable_test)
            ok("Configure completed")
        elif args.command == "kernel":
            banner("TilekarOS Kernel Build")
            configure(args.arch, enable_test=args.enable_test)
            build_target("myos.kernel")
            ok("Kernel build completed")
        elif args.command == "sysroot":
            banner("TilekarOS Sysroot Build")
            ensure_sysroot(args.arch)
            ok("Sysroot build completed")
        elif args.command == "userland":
            ensure_sysroot(args.arch)
            ok("Userland build completed")
        elif args.command == "iso":
            banner("TilekarOS ISO Build")
            configure(args.arch, enable_test=args.enable_test)
            build_target("iso")
            ok("ISO build completed")
        elif args.command in {"run", "run_iso", "run_disk"}:
            run_qemu(
                args.arch,
                args.vm,
                args.drives,
                args.command,
                enable_test=args.enable_test,
            )
        elif args.command == "export_drives":
            banner("TilekarOS Drive Export")
            vm_dir, drives = ensure_vm_workspace(args.vm, args.drives)
            export_drives(vm_dir, drives)
            ok("Drive export completed")
        elif args.command == "comp":
            ensure_sysroot(args.arch)
            compile_user_program(args.arch, args.file, args.out)
        elif args.command == "clean":
            banner("TilekarOS Clean")
            shutil.rmtree(str(BUILD_DIR), ignore_errors=True)
            shutil.rmtree(str(SYSROOT), ignore_errors=True)
            vm_root = ROOT / args.vm
            preserve_dir = vm_root / "exported_drives" / "boot"

            # Collect names of files to preserve
            preserve_names = set()
            if preserve_dir.exists():
                preserve_names = {
                    p.name for p in preserve_dir.glob("*.c") if p.is_file()
                }

            # Walk and delete everything except preserved files
            for root, dirs, files in os.walk(str(vm_root), topdown=False):
                root_path = ROOT.__class__(root)  # keep Path compatibility

                # Delete files
                for name in files:
                    if root_path == preserve_dir and name in preserve_names:
                        continue
                    try:
                        os.remove(os.path.join(root, name))
                    except OSError:
                        pass

                # Delete directories if empty
                for name in dirs:
                    dirpath = os.path.join(root, name)
                    try:
                        os.rmdir(dirpath)
                    except OSError:
                        pass

            ok("Clean completed")
        return 0
    except (subprocess.CalledProcessError, ValueError) as exc:
        fail("build.py error: {0}".format(exc))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
