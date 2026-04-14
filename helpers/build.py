#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List


ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
SYSROOT = ROOT / "sysroot"


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


def _style(text: str, *codes: str) -> str:
    if not USE_COLOR:
        return text
    return "".join(codes) + text + RESET


def banner(title: str) -> None:
    line = "═" * 68
    print(_style(f"\n{line}", CYAN))
    print(_style(f"🚀 {title}", BOLD, CYAN))
    print(_style(line, CYAN))


def step(message: str) -> None:
    print(f"{_style('🔹', BLUE)} {_style(message, BOLD)}")


def info(message: str) -> None:
    print(f"{_style('ℹ️ ', CYAN)} {message}")


def ok(message: str) -> None:
    print(f"{_style('✅', GREEN)} {_style(message, GREEN)}")


def warn(message: str) -> None:
    print(f"{_style('⚠️ ', YELLOW)} {_style(message, YELLOW)}")


def fail(message: str) -> None:
    print(f"{_style('❌', RED)} {_style(message, RED)}")


def run(cmd: List[str], cwd: Path | None = None) -> None:
    print(_style(f"   $ {' '.join(cmd)}", DIM))
    subprocess.run(cmd, cwd=cwd or ROOT, check=True)


def run_allow_failure(cmd: List[str], cwd: Path | None = None) -> int:
    print(_style(f"   $ {' '.join(cmd)}", DIM))
    proc = subprocess.run(cmd, cwd=cwd or ROOT, check=False)
    return proc.returncode


def first_drive_name(drives: str) -> str:
    return parse_drives(drives)[0].name


@dataclass
class Drive:
    name: str
    size_mb: int
    interface: str


def parse_drives(drives: str) -> List[Drive]:
    out: List[Drive] = []
    for raw in (x.strip() for x in drives.split(",") if x.strip()):
        parts = raw.split(":")
        name = parts[0]
        size_mb = 24
        interface = "ide"
        if len(parts) >= 2 and parts[1]:
            size_mb = int(parts[1])
        if len(parts) >= 3 and parts[2]:
            interface = parts[2].lower()
        if interface not in {"ide", "ahci"}:
            raise ValueError(f"Unsupported attach_type '{interface}' for drive '{name}'")
        out.append(Drive(name=name, size_mb=size_mb, interface=interface))
    if not out:
        out.append(Drive(name="boot", size_mb=24, interface="ide"))
    return out


def configure(arch: str) -> None:
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    cmake_cache = BUILD_DIR / "CMakeCache.txt"
    cmake_cmd = [
        "cmake",
        "-S",
        str(ROOT),
        "-B",
        str(BUILD_DIR),
        f"-DOS_ARCH={arch}",
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_ASM_NASM_COMPILER=nasm",
    ]
    toolchain = ROOT / "cmake" / "toolchains" / f"{arch}.cmake"
    if cmake_cache.exists():
        text = cmake_cache.read_text(encoding="utf-8", errors="ignore")
        if "CMAKE_C_COMPILER:FILEPATH=/usr/bin/cc" in text:
            shutil.rmtree(BUILD_DIR, ignore_errors=True)
            BUILD_DIR.mkdir(parents=True, exist_ok=True)
            cmake_cache = BUILD_DIR / "CMakeCache.txt"
    if toolchain.exists():
        cmake_cmd.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain}")
    elif cmake_cache.exists():
        text = cmake_cache.read_text(encoding="utf-8", errors="ignore")
        if "CMAKE_TOOLCHAIN_FILE" in text:
            cmake_cache.unlink()

    if cmake_cache.exists():
        cache_text = cmake_cache.read_text(encoding="utf-8", errors="ignore")
        arch_ok = f"OS_ARCH:UNINITIALIZED={arch}" in cache_text or f"OS_ARCH:STRING={arch}" in cache_text
        cc_ok = "CMAKE_C_COMPILER:UNINITIALIZED=clang" in cache_text or "CMAKE_C_COMPILER:FILEPATH=/usr/bin/clang" in cache_text
        nasm_ok = "CMAKE_ASM_NASM_COMPILER:UNINITIALIZED=nasm" in cache_text or "CMAKE_ASM_NASM_COMPILER:FILEPATH=/usr/bin/nasm" in cache_text
        if arch_ok and cc_ok and nasm_ok:
            step(f"Using existing CMake configuration (arch={arch})")
            return

    step(f"Configuring CMake (arch={arch})")
    run(cmake_cmd)


def build_target(target: str) -> None:
    step(f"Building target: {target}")
    run(["cmake", "--build", str(BUILD_DIR), "--target", target])


def sync_sysroot_headers() -> None:
    step("Syncing sysroot headers (libc + kernel)")
    include_dir = SYSROOT / "usr" / "include"
    include_dir.mkdir(parents=True, exist_ok=True)
    run(["cmake", "-E", "copy_directory", str(ROOT / "libc" / "include"), str(include_dir)])
    run(["cmake", "-E", "copy_directory", str(ROOT / "kernel" / "include"), str(include_dir)])


def ensure_sysroot(arch: str, do_configure: bool = True) -> None:
    step("Preparing sysroot")
    if do_configure:
        configure(arch)
    build_target("libc")
    build_target("sysroot_extras")
    sync_sysroot_headers()


def collect_workspace_c_files(vm_dir: Path) -> List[Path]:
    return sorted(vm_dir.glob("*.c"))


def create_empty_fat_image(img: Path, size_mb: int) -> None:
    step(f"Creating empty FAT image: {img.name} ({size_mb}MB)")
    run(["dd", "if=/dev/zero", f"of={img}", "bs=1M", f"count={size_mb}", "status=none"])
    run(["mkfs.fat", str(img)])


def create_image_from_export(img: Path, size_mb: int, export_dir: Path) -> None:
    step(f"Recreating {img.name} from exported content: {export_dir}")
    create_empty_fat_image(img, size_mb)
    if export_dir.exists():
        items = [str(p) for p in export_dir.iterdir()]
        if items:
            # Pass items explicitly since subprocess.run doesn't expand globs
            run(["mcopy", "-i", str(img), "-snD", "o", "-s"] + items + ["::/"])


def is_fat_image_healthy(img: Path) -> bool:
    return run_allow_failure(["minfo", "-i", str(img), "::"]) == 0


def write_drive_meta(meta_path: Path, size_mb: int) -> None:
    meta_path.write_text(str(size_mb), encoding="ascii")


def read_drive_meta(meta_path: Path) -> int | None:
    if not meta_path.exists():
        return None
    try:
        return int(meta_path.read_text(encoding="ascii").strip())
    except ValueError:
        return None


def ensure_vm_workspace(vm: str, drives_cfg: str) -> tuple[Path, List[Drive]]:
    step(f"Preparing VM workspace: {vm}")
    vm_dir = ROOT / vm
    drives_dir = vm_dir / "drives"
    export_root = vm_dir / "exported_drives"
    drives_dir.mkdir(parents=True, exist_ok=True)
    export_root.mkdir(parents=True, exist_ok=True)

    drives = parse_drives(drives_cfg)
    step(f"Drive configuration: {drives_cfg}")
    for d in drives:
        img = drives_dir / f"{d.name}.img"
        meta = drives_dir / f"{d.name}.meta"
        export_dir = export_root / d.name

        # Always recreate from export if it exists to ensure sync
        if export_dir.exists() and any(export_dir.iterdir()):
            create_image_from_export(img, d.size_mb, export_dir)
            write_drive_meta(meta, d.size_mb)
            continue

        old_size = read_drive_meta(meta)
        recreate = (not img.exists()) or (old_size != d.size_mb)
        if img.exists() and not recreate and not is_fat_image_healthy(img):
            warn(f"Detected FAT corruption in {img.name}; recreating image")
            recreate = True
        if recreate:
            create_empty_fat_image(img, d.size_mb)
            write_drive_meta(meta, d.size_mb)
    return vm_dir, drives


def inject_boot_payload(vm_dir: Path, boot_img: Path, kernel_binary: Path) -> None:
    step(f"Injecting kernel and VM C files into drive: {boot_img.name}")
    # /boot may already exist in persistent VM images; treat that as non-fatal.
    run_allow_failure(["mmd", "-i", str(boot_img), "-D", "o", "::/boot"])
    run(["mcopy", "-i", str(boot_img), "-D", "o", str(kernel_binary), "::/boot/myos.kernel"])
    for cfile in collect_workspace_c_files(vm_dir):
        run(["mcopy", "-i", str(boot_img), "-D", "o", str(cfile), f"::/{cfile.stem}"])


def export_drives(vm_dir: Path, drives: List[Drive]) -> None:
    step("Exporting drives to exported_drives")
    drives_dir = vm_dir / "drives"
    export_root = vm_dir / "exported_drives"
    export_root.mkdir(parents=True, exist_ok=True)
    for d in drives:
        img = drives_dir / f"{d.name}.img"
        if not img.exists():
            continue
        export_dir = export_root / d.name
        export_dir.mkdir(parents=True, exist_ok=True)
        attempts = [
            ["mcopy", "-i", str(img), "-snD", "o", "::/*", f"{export_dir}/"],
            ["mcopy", "-i", f"{img}@@512", "-snD", "o", "::/*", f"{export_dir}/"],
        ]
        exported = False
        for cmd in attempts:
            print(_style(f"   $ {' '.join(cmd)}", DIM))
            proc = subprocess.run(cmd, cwd=ROOT, check=False, capture_output=True, text=True)
            if proc.returncode == 0:
                exported = True
                break
        if not exported:
            warn(f"Skipping export for {d.name}.img (FAT not readable)")


def qemu_drive_flags(vm_dir: Path, drives: List[Drive]) -> List[str]:
    flags: List[str] = []
    has_ahci = False
    ahci_num = 0
    ide_num = 0
    for _, d in enumerate(drives):
        boot_prop = ",bootindex=0" if (ide_num + ahci_num) == 0 else ""
        img = vm_dir / "drives" / f"{d.name}.img"
        if d.interface == "ahci":
            if not has_ahci:
                flags += ["-device", "ich9-ahci,id=ahci0"]
                has_ahci = True
            flags += ["-drive", f"file={img},format=raw,if=none,id=ahci_d{ahci_num}"]
            flags += ["-device", f"ide-hd,bus=ahci0.{ahci_num},drive=ahci_d{ahci_num}{boot_prop}"]
            ahci_num += 1
            continue
        bus = ide_num // 2
        unit = ide_num % 2
        if ide_num < 4:
            # Legacy "if=ide" syntax does not accept bootindex on many QEMU versions.
            flags += ["-drive", f"file={img},format=raw,if=ide,bus={bus},unit={unit}"]
        else:
            extra_idx = ((ide_num - 4) // 4) + 1
            extra_drive_idx = (ide_num - 4) % 4
            extra_bus = extra_drive_idx // 2
            extra_unit = extra_drive_idx % 2
            if extra_drive_idx == 0:
                flags += ["-device", f"piix4-ide,id=extide{extra_idx}"]
            flags += ["-drive", f"file={img},format=raw,if=none,id=d{ide_num}"]
            flags += [
                "-device",
                f"ide-hd,bus=extide{extra_idx}.{extra_bus},drive=d{ide_num},unit={extra_unit}{boot_prop}",
            ]
        ide_num += 1
    return flags


def run_qemu(arch: str, vm: str, drives_cfg: str, mode: str) -> None:
    banner(f"TilekarOS Run Pipeline ({mode})")
    step(f"Run mode: {mode}")
    vm_dir, drives = ensure_vm_workspace(vm, drives_cfg)
    configure(arch)
    build_target("myos.kernel")
    ensure_sysroot(arch, do_configure=False)

    kernel_binary = BUILD_DIR / "kernel" / "myos.kernel"
    boot_img = vm_dir / "drives" / f"{first_drive_name(drives_cfg)}.img"

    qemu_cmd = [f"qemu-system-{arch}"]
    if mode == "run":
        qemu_cmd += ["-kernel", str(kernel_binary)]
    elif mode == "run_iso":
        build_target("iso")
        qemu_cmd += ["-boot", "d", "-cdrom", str(BUILD_DIR / "myos.iso")]
    elif mode == "run_disk":
        inject_boot_payload(vm_dir, boot_img, kernel_binary)
        isodir = BUILD_DIR / "disk_isodir"
        shutil.rmtree(isodir, ignore_errors=True)
        run(["cmake", "-E", "make_directory", str(isodir / "boot" / "grub")])
        run(["cmake", "-E", "copy", str(kernel_binary), str(isodir / "boot" / "myos.kernel")])
        run(["cmake", "-E", "copy", str(ROOT / "grub.cfg"), str(isodir / "boot" / "grub" / "grub.cfg")])
        run(["grub-mkrescue", "-o", str(boot_img), str(isodir)])
        qemu_cmd += ["-boot", "c"]
    else:
        raise ValueError(f"Unsupported run mode {mode}")

    qemu_cmd += qemu_drive_flags(vm_dir, drives)
    qemu_cmd += ["-d", "guest_errors,unimp", "-D", str(vm_dir / "qemu.log")]
    qemu_cmd += ["-serial", f"file:{vm_dir / 'serial.log'}"]
    qemu_cmd += ["-monitor", "stdio"]
    step("Launching QEMU")
    info("QEMU command:")
    print(_style("  " + " ".join(qemu_cmd), MAGENTA))
    run(qemu_cmd)
    export_drives(vm_dir, drives)
    ok("Run pipeline completed")


def compile_user_program(arch: str, src_file: str, out_file: str | None) -> None:
    banner("TilekarOS Userspace Compilation")
    if not src_file:
        raise ValueError("comp requires --file <path-to-c-file>")
    src = Path(src_file)
    out = Path(out_file) if out_file else src.with_suffix("")
    out.parent.mkdir(parents=True, exist_ok=True)
    step(f"Compiling userspace program: {src} -> {out}")
    ensure_sysroot(arch)
    run(
        [
            "clang",
            f"--target={arch}-elf",
            f"--sysroot={SYSROOT}",
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
    ok(f"Built userspace executable: {out}")


def main() -> int:
    parser = argparse.ArgumentParser(description="TilekarOS Python build orchestrator")
    parser.add_argument("command", choices=["configure", "kernel", "sysroot", "iso", "run", "run_iso", "run_disk", "export_drives", "comp", "clean"])
    parser.add_argument("--arch", default=os.environ.get("ARCH", "i386"))
    parser.add_argument("--vm", default=os.environ.get("VM", "VirtualMachine"))
    parser.add_argument("--drives", default=os.environ.get("DRIVES", "boot:24:ide"))
    parser.add_argument("--file", default=os.environ.get("FILE"))
    parser.add_argument("--out", default=os.environ.get("OUT"))
    args = parser.parse_args()

    try:
        if args.command == "configure":
            banner("TilekarOS Configure")
            configure(args.arch)
            ok("Configure completed")
        elif args.command == "kernel":
            banner("TilekarOS Kernel Build")
            configure(args.arch)
            build_target("myos.kernel")
            ok("Kernel build completed")
        elif args.command == "sysroot":
            banner("TilekarOS Sysroot Build")
            ensure_sysroot(args.arch)
            ok("Sysroot build completed")
        elif args.command == "iso":
            banner("TilekarOS ISO Build")
            configure(args.arch)
            build_target("iso")
            ok("ISO build completed")
        elif args.command in {"run", "run_iso", "run_disk"}:
            run_qemu(args.arch, args.vm, args.drives, args.command)
        elif args.command == "export_drives":
            banner("TilekarOS Drive Export")
            vm_dir, drives = ensure_vm_workspace(args.vm, args.drives)
            export_drives(vm_dir, drives)
            ok("Drive export completed")
        elif args.command == "comp":
            compile_user_program(args.arch, args.file, args.out)
        elif args.command == "clean":
            banner("TilekarOS Clean")
            shutil.rmtree(BUILD_DIR, ignore_errors=True)
            shutil.rmtree(SYSROOT, ignore_errors=True)
            shutil.rmtree(ROOT / args.vm, ignore_errors=True)
            ok("Clean completed")
        return 0
    except (subprocess.CalledProcessError, ValueError) as exc:
        fail(f"build.py error: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
