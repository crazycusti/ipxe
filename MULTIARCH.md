# Experimental iPXE multiarchitecture fork

This fork adds native SPARC32/OpenPROM, SPARC64, PPC32/PPC64 BE/PPC64 LE
IEEE 1275, narrow HPPA/PDC and Alpha/SRM clients, and an IA-64 EFI client.
It is intended as a foundation for a shared netboot environment and future
Mezereon loading. The upstream tree and its licences remain intact.

Development branch: `feature/ieee1275-multiarch`.
Initial upstream base: `c51ca76` (the full revision is available with
`git merge-base HEAD upstream/master`).

## Verified scope

| Target | Self-tests | Network/menu | Linux handoff |
|---|---|---|---|
| SPARC32 / OpenBIOS SS-5, V8 | 13,306 | Native SBus LANCE, OpenPROM DVMA, DHCP/HTTP/SHA256 and downloaded menu | Not implemented |
| SPARC64 / OpenBIOS sun4u | 13,306 | Sabre PCI/Intel, DHCP/HTTP/SHA256 and downloaded menu | Gentoo live shell; kernel/initramfs fetched by iPXE |
| PPC32 / SLOF pseries POWER9 | 13,306 | Firmware TFTP, iPXE DHCP/HTTP/SHA256/menu | Gentoo PPC32 live shell on Mac99/G4 |
| PPC64 BE / SLOF pseries POWER9 | 13,306 | Firmware TFTP, iPXE DHCP/HTTP/SHA256/menu | Gentoo ibmpower live shell |
| PPC64 LE / SLOF pseries POWER9 | 13,306 | Firmware TFTP, iPXE DHCP/HTTP/SHA256/menu | Gentoo PPC64LE live shell |
| PowerMac / OpenBIOS Mac99, 970 | Native PPC64 BE; additional Mac99 self-test coverage | Native UniNorth/U3 PCI/Intel | Gentoo live shell through framebuffer/keyboard; official PPC64 serial driver is a module |
| HPPA / PDC B160L | 13,306 | SCSI/PALO, Dino/Intel DHCP/HTTP/SHA256/menu | Gentoo PA1.1 live shell |
| PA-RISC 2.0 / PDC C3700 | Narrow PDC client; Astro/Elroy PCI | SCSI/PALO and native Intel | Genuine ELF64 Linux kernel/initramfs entry; see release evidence for live userspace |
| Alpha EV6+ / QEMU clipper | 13,306 | Tsunami/Intel DHCP/HTTP/SHA256/menu; direct SRM ELF bootstrap | Gentoo Alpha live shell |
| IA-64 / separate experimental EFI emulator | 13,306 | Intel/SNP network and EFI application chaining | OS kernel handoff not qualified |

There is no physical-hardware qualification. These binaries have not replaced
the operational PC, JavaStation, HPPA or Solaris boot files. Linux qualification is limited to the exact cases above. Solaris, HP-UX and
Mezereon kernel handoffs through these new clients remain unqualified.

The SPARC64 bootstrap currently uses virtual/physical address `0x40004000`
to match the QEMU OpenBIOS direct-kernel entry. Its QEMU fixture therefore
uses 2 GiB RAM. Relocation, a suitable general OpenBoot network container and
lower-memory machines need further work before physical deployment.

## Structure

- `src/arch/sparc32/`: actual V8 entry, register windows and local compiler
  helpers, plus a bounded OpenPROM vector adapter for shared firmware services.
- `src/arch/hppa/`: PA-RISC 1.1 entry, callee-saved state, linker layout and ABI.
- `src/interface/pdc/`: PDC/IODC console, calibrated interval timer, bounded
  image-buffer heap and a firmware-confirmed narrow Dino PCI backend.
- `src/arch/sparc64/`: V9 entry, register windows, setjmp/longjmp, compiler ABI
  and linker layout.
- `src/arch/powerpc/`: PPC32 entry, preserved registers, linker layout and ABI.
- `src/interface/openfirmware/`: shared client interface, console, timer,
  page-aligned claim/release image buffers and raw Ethernet firmware adapter.
- `src/include/ipxe/portable/`: shared scalar primitives, byte order,
  bit operations and 32-bit-limb big integers.
- Narrow portability fixes in DNS, EUI-64, TLS wire-field types and the
  checksum test reference avoid unaligned integer accesses on SPARC.

The external allocator implements iPXE's zero-size `NOWHERE` contract,
preserves the previous allocation on failure, checks size arithmetic and
obtains page-aligned buffers from firmware. Compiler helper routines are
statically linked from the matching cross-compiler's `libgcc` for SPARC64,
PowerPC and HPPA. SPARC32 uses local V8 helpers because distribution multilib
`libgcc` targets V8+ and cannot run on a V8-only CPU. No libc or
other operating-system runtime is linked into the native image.

The initial firmware platforms enable HTTP/TFTP. HTTPS is disabled by default
until qualified entropy and wall-clock providers exist. The cryptographic
self-tests still exercise the underlying algorithms and wire formats.

## Build

On an Ubuntu build host, install the matching cross compiler and QEMU:

```sh
sudo apt-get install gcc-sparc64-linux-gnu gcc-powerpc-linux-gnu gcc-hppa-linux-gnu palo qemu-system-sparc qemu-system-ppc qemu-system-hppa
make -C src CROSS=sparc64-linux-gnu- bin-sparc32-openfirmware/ipxe.of
make -C src CROSS=sparc64-linux-gnu- bin-sparc64-openfirmware/ipxe.of
make -C src CROSS=powerpc-linux-gnu- bin-powerpc-openfirmware/ipxe.of
make -C src CROSS=hppa-linux-gnu- bin-hppa-pdc/ipxe.lif
```

Use explicit target names. The existing x86 default and operational PC
loaders are independent of these new targets. `none.of` omits the network
adapter; `tests.of` executes upstream's native self-test image. A normal iPXE
script can be embedded using `EMBED=/absolute/path/to/script.ipxe`.

## Reproducible tests

Each command requires a fresh output directory:

```sh
python3 tools/test-native.py sparc32 selftests /tmp/ipxe-sparc32-tests --qemu /path/to/qemu-10.1/qemu-system-sparc
python3 tools/test-native.py sparc32 input /tmp/ipxe-sparc32-input --qemu /path/to/qemu-10.1/qemu-system-sparc
python3 tools/test-native.py hppa selftests /tmp/ipxe-hppa-tests
python3 tools/test-native.py hppa input /tmp/ipxe-hppa-input
python3 tools/test-native.py hppa network /tmp/ipxe-hppa-network
python3 tools/test-native.py sparc64 selftests /tmp/ipxe-sparc64-tests
python3 tools/test-native.py sparc64 menu /tmp/ipxe-sparc64-menu
python3 tools/test-native.py powerpc selftests /tmp/ipxe-ppc-tests
python3 tools/test-native.py powerpc network /tmp/ipxe-ppc-network
python3 tools/test-pc.py bios /tmp/ipxe-pc-bios
python3 tools/test-pc.py uefi /tmp/ipxe-pc-uefi
```

The tool records exact build/boot commands, binary hashes, serial evidence,
HTTP requests and a strict result. The network case hosts two temporary files
on localhost, checks a 16 KiB download's SHA256 inside iPXE and executes the
downloaded menu. It uses QEMU user networking and changes no LAN services.

The PowerPC self-test also traverses real firmware TFTP. The SPARC test uses
QEMU's direct ELF bootstrap; it is not a SPARC firmware network-boot proof.
Self-test binaries are independent of the normal operational build.

The PC regression also builds from this fork and loads the resulting binary
through isolated firmware PXE before checking its embedded menu. It requires
KVM and the Ubuntu OVMF files named in the tool. The x86-64 Linux self-test
target (`bin-x86_64-linux/tests.linux`) additionally passes all 12,360 tests.

## Mezereon and further ports

Keep CPU primitives, firmware services and executable loading separate.
The existing Mezereon boot information must remain authoritative: do not
invent an incompatible second boot-info structure in this fork. The next
loading milestone needs a documented handoff for kernel entry, boot arguments,
initramfs, memory reservations and the live firmware callback.

SPARC32 has its own V8 entry/ABI and cannot use the SPARC64 image. HPPA uses
PDC/IODC and a LIF/PALO entry; it does not use the IEEE 1275 backend.
Native PPC64 and little-endian kernel entry need separate qualification even
when the initial firmware client is PPC32.

The first follow-up milestones are an appropriate SPARC network container,
firmware packet I/O on real OpenBoot, low-memory/relocation support and
architecture-specific executable handoff. Native PCI/DMA backends are a
separate alternative for firmware without packet I/O. Solaris/HP-UX retain
their native installation protocols and media requirements.

Changes are maintained in this fork. Upstream submissions follow the human
ownership and review requirements in `AGENTS.md` and `CONTRIBUTING.md`.

## SPARC32 and HPPA qualification details

SPARC32 is tested on a V8-only SS-5 CPU, using QEMU 10.1. QEMU 8.2's
SPARC signed division helper uses the unconverted divisor in its final
operation, producing incorrect results for negative divisors. This is visible
in the unchanged time self-tests. Compare the upstream
[8.2.2 helper](https://github.com/qemu/qemu/blob/v8.2.2/target/sparc/helper.c)
and [10.1.0 helper](https://github.com/qemu/qemu/blob/v10.1.0/target/sparc/helper.c).
The newer emulator is built separately; installed QEMU and existing VMs stay
independent. `--qemu` records the chosen emulator and version.

The SPARC32 ELF uses virtual entry 0x4000 and a Linux-style physical-address
bias of 0xf0000000 for QEMU's direct kernel loader. A general OpenPROM network
container and JavaStation hardware qualification remain open. The adapter
uses v2 device read for console input and synthesises the required /chosen
stdin/stdout properties from the authoritative OpenPROM boot arguments.
It preserves PROM trap handlers and permits the level-14 clock interrupt.
Register windows are flushed by window rotation, without assuming a software
trap service. No V8+ or V9 compiler-library objects are linked.

HPPA runs through firmware, SCSI and PALO from a fresh read-only LIF image.
The normal `.pdc` output is the ELF payload and `.lif` includes PALO. Code
starts at 1 MiB; its upward-growing stack and 16 MiB external-image heap are
reserved in the ELF BSS. This intentionally bounded allocator does not claim
arbitrary firmware or PALO memory. Firmware calibration supplies the CR16
timer frequency. IODC input and output use Page Zero's console descriptors.

The network fixture adds an Intel 82540EM (`e1000`) to the emulated Dino PCI
bridge. PDC must confirm the known Dino identifier before any bridge MMIO.
Only its first PCI bus and firmware-established MMIO window are supported.
This does not qualify a real HP 712's integrated LAN, other host bridges,
cache/DMA coherency on physical HPPA, or firmware BOOTP/TFTP loading of LIF.
The proven path is SCSI/PALO into iPXE, then native DHCP/HTTP and a menu.

Additional narrow portability changes preserve eight-byte heap alignment
where the ABI requires it, align the hash compression scratch copy, access
packed GCM lengths through memcpy, mark the in-packet TCP queue header packed,
and use GNU `.set` aliases (also accepted by PA-RISC GAS).
Tests and assertions are not disabled. `input` verifies actual serial key
input: it must change away from a deliberately wrong default selection.
`--trace` bounds QEMU exception logs to 32 MiB; `--debug` enables named iPXE
objects. The old `test-ieee1275.py` command remains a compatibility launcher.


## IA-64 EFI qualification

IA-64 is an EFI target with DHCP architecture ID **2**. It does not require
identifying a particular Itanium server model before starting the generic
port. Use `bin-ia64-efi/snp.efi` with firmware SNP for deployment;
`intel.efi` is the native PCI alternative used to bootstrap the emulator.
Native firmware drivers, EFI console and EFI application chaining are shared
with the existing EFI backend. Physical IA-64 hardware remains unqualified.

The entry point is a PE32+ two-word function descriptor. The linker and
GNU-EFI-derived startup handle the IA-64 GP/GOT, ELF runtime relocations and
canonical function descriptors. Function-pointer identity, including aliases,
is required by iPXE's interface dispatch and is explicitly self-tested.
The imported assembly retains its original BSD notices; provenance is in
`src/arch/ia64/README`. GNU assembler uses automatic dependency scheduling.
The EFI entry relocation descriptor is page-aligned. Failed relocations use
EFI error statuses with the native high bit set.

An IA-64 GCC 12.2 ELF toolchain and binutils 2.39 can build the port:

```sh
make -C src CROSS=/path/to/ia64-linux- bin-ia64-efi/snp.efi
make -C src CROSS=/path/to/ia64-linux- bin-ia64-efi/intel.efi
make -C src CROSS=/path/to/ia64-linux- bin-ia64-efi/tests.efi
python3 tools/test-ia64.py selftests --output /tmp/ia64-tests --qemu /path/to/qemu-system-ia64 --firmware /path/to/ia64-firmware.bin --cross /path/to/ia64-linux-
python3 tools/test-ia64.py input --output /tmp/ia64-input --qemu /path/to/qemu-system-ia64 --firmware /path/to/ia64-firmware.bin --cross /path/to/ia64-linux-
python3 tools/test-ia64.py network --output /tmp/ia64-network --qemu /path/to/qemu-system-ia64 --firmware /path/to/ia64-firmware.bin --cross /path/to/ia64-linux-
python3 tools/test-ia64.py snp-network --output /tmp/ia64-snp --qemu /path/to/qemu-system-ia64 --firmware /path/to/ia64-firmware.bin --cross /path/to/ia64-linux-
```

The test emulator is the experimental third-party
[syunnPC IA-64 QEMU fork](https://github.com/syunnPC/qemu-system-ia64), pinned
at `9902cf66038403ef8ec3621126b540824c12f12e` (reported QEMU version 11.0.50).
It is independent of upstream QEMU. Its firmware does not implement PXE/SNP
network bootstrap, and only enumerated the built-in storage/graphics devices
through EFI PCI I/O. The fixture patch
`tools/patches/qemu-ia64-vpc-efi-pci-network.patch` exposes the VPC machine's
existing e1000 at PCI 00:06.0 and polls its cooperative timer events at TPL
transitions. This patch changes the **test firmware**, not iPXE's network
protocols. It is limited to that VPC adapter; it adds no generic firmware PXE
implementation. Apply with `git apply --unidiff-zero /path/to/qemu-ia64-vpc-efi-pci-network.patch`
and rebuild the firmware with the pinned IA-64 cross toolchain.

Tests boot a fresh FAT disk through EFI. Disk writes use a temporary overlay;
no user disk or host network configuration is attached. Native iPXE performs
DHCP and HTTP. The `snp-network` case downloads a second IA-64 EFI executable,
starts it through LoadImage/StartImage, and verifies that this second iPXE
uses the first stage's SNP protocol to obtain DHCP, fetch HTTP and TFTP
payloads, check both SHA256 values and execute a downloaded menu. A pcap,
serial logs, copied firmware, hashes and exact commands are retained.

`deployed-menu --chain-efi /path/to/deployed.efi` checks the byte-identical
installed EFI loader and its actual served menu, including the default return
to firmware. Full firmware PXE, an operating-system kernel handoff, Secure
Boot, and a complete Gentoo/HP-UX/OpenVMS installation are **not** qualified by
these tests. The mirrored IA-64 Gentoo media remains a separate archive.

The added generic function-pointer test also passes on HPPA, SPARC32,
SPARC64, PPC32 and x86-64. Current native suites have 12,244 checks;
x86-64/Linux has 12,364. Original r1/r2 evidence records the older suite
counts before these four additional checks.


## PPC64, PA-RISC 2.0 and Alpha extension (r4, 2026-10-17)

PowerPC64 BE uses ELFv1 function descriptors; LE uses ELFv2. Both keep the
IEEE 1275 CIF argument block in big-endian 32-bit cells, even with 64-bit
pointers in C. The assembly CIF bridge saves native register/MSR state,
reserves the firmware caller argument area, switches to the firmware's
32-bit BE mode and translation state, and restores the native client.
The BE dependency relocations live outside `.opd`, so GNU ld recognises the
function descriptors and the interface function identity checks still pass.

For Apple firmware, `.mac` is an ELF32 container holding the actual PPC64
code and data. Its entry switches a 32-bit firmware entry to the 64-bit
client. It is not a PPC32 executable. The Mac99 fixture translates only
its disposable ELF physical addresses to account for QEMU's 16 MiB load
bias. Published `.mac` files retain their normal physical addresses.
QEMU Mac99 with U3 and a 970 CPU covers that model, not every physical G5.
Apple firmware packet I/O is retained; the emulated Intel card is tested
through a host-bridge-checked UniNorth/U3 CFA0 PCI backend. The onboard G5
Sungem and USB/ADB keyboards have no physical qualification.

PA-RISC 2.0 uses the existing narrow PDC/PALO calling convention. PDC IODC
must confirm each Dino/Elroy bridge before register access. Astro/Elroy has
four independently addressed roots and PCI-endian selectors. Both B160L
and C3700 complete actual network/menu tests. This does not turn the
loader into a wide HPPA64 executable or qualify a 64-bit OS entry.

Alpha currently targets EV6 and later with BWX, on the Tsunami chipset for
native PCI. It enters with OSF PAL already selected, uses the HWRPB CRB
console, a bounded image-owned heap, RPCC timing, Unix/VMS callback glue
and compiler-convention integer division helpers. The direct PCI DMA
window uses an unused window 0..2 and restores the original registers
on shutdown; window 3 is scatter/gather-only. QEMU PALcode has a blocking
GETC implementation, so the confirmed QEMU/Tsunami UART is polled before
GETC. Other SRM firmware uses the callback's own nonblocking status.
The payload is placed at physical 0x01010000 to leave the SRM loader and
aboot's parameter area free. EV4/EV5, ARC/AlphaBIOS and other PCI chipsets
are not qualified.

Example builds (GNU cross compiler prefixes may be overridden):

```sh
make -C src CROSS=powerpc64-linux-gnu- bin-powerpc64-openfirmware/ipxe.of
make -C src CROSS=powerpc64-linux-gnu- bin-powerpc64-openfirmware/ipxe.mac
make -C src CROSS=powerpc64le-linux-gnu- bin-powerpc64le-openfirmware/ipxe.of
make -C src CROSS=alpha-linux-gnu- bin-alpha-srm/ipxe.srm
make -C src CROSS=alpha-linux-gnu- ABOOT=/path/to/netabootwrap bin-alpha-srm/ipxe.bootp
```

The Alpha `.srm` file is an ELF payload, not an SRM network container.
`.bootp` wraps a stripped, gzip-compressed ELF with upstream aboot's
`netabootwrap`. The pinned aboot source, ELF, compressed payload and wrapper
must be distributed together with their licences/source archive. QEMU's
PALcode has no real SRM BOOTP implementation: only the inner ELF is runtime
qualified. Wrapper layout/decompression can be checked here; its first SRM
firmware boot needs hardware. Do not announce an SRM PXE pass from the
QEMU direct ELF result.

The Alpha tests also exercise unaligned AES table columns, CBC buffers and
byte-aligned FDT images. Those accesses now use packed tokens or memcpy;
no checks or assertions were removed. `ASM_EQU` selects the assembler's
symbol-assignment directive without changing other architectures' syntax.

Additional test examples:

```sh
python3 tools/test-native.py powerpc64 selftests /tmp/ppc64-tests --cross=powerpc64-linux-gnu-
python3 tools/test-native.py powerpc64 network /tmp/g5-tests --machine mac99 --cpu 970 --cross=powerpc64-linux-gnu-
python3 tools/test-native.py powerpc64le network /tmp/ppc64le-tests
python3 tools/test-native.py hppa network /tmp/pa20-tests --machine C3700
python3 tools/test-native.py alpha selftests /tmp/alpha-tests --cross=alpha-linux-gnu-
python3 tools/test-native.py alpha network /tmp/alpha-network --cross=alpha-linux-gnu- --capture
```

`--binary` tests an exact published loader. `deployed-menu --binary ...`
asserts that this loader downloads the real central HTTP menu and displays
its native architecture menu. It does not emulate an OS start.

Final r4 evidence is under `/data/netboot/tests/ipxe-fork/r4-final-*`;
failed earlier attempts remain separate. All eight native architecture
self-test runs pass 12,244 checks each; x86-64/Linux passes 12,364. The
separate IA64 emulator remains necessary. No physical hardware or Linux,
Solaris, HP-UX, OpenVMS or Mezereon handoff is claimed for these ports.


## Native network and Linux handoff (r5, 2026-10-17)

The main emulator used for r5 is `/opt/qemu/11.1.2/bin`. Stock 11.1.2 also
runs the SPARC V8 tests; the earlier 10.1 division workaround is unnecessary
for this test version. IA-64 still requires the separate experimental fork.
The x86-64/Linux test image passes 13,426 assertions. The native targets each
pass 13,306, including 1,062 new ELF parser assertions.

`native_elf_parse()` decodes ELF32/ELF64 in either byte order, rejects truncated
headers/tables/segments, arithmetic overflow, overlapping destination ranges
and entry points in BSS. It does not execute images itself. The architecture
adapters select their machine type and validate/reserve the destinations.

The PowerPC loader preserves the original firmware callback and entry mode.
It reserves kernel and initramfs separately, outside the low RAM overwritten
by Linux's relocation. PPC32 also keeps the initramfs/device tree below its
initial 256 MiB BAT mapping. PPC64 BE and LE take the real 32-bit BE firmware
entry required by `prom_init`. Mac99 G5 testing uses the ELF32 container for
the native 64-bit client and the normal ELF64 Gentoo kernel.

Alpha uses the aboot Linux parameter-page contract and a firmware-described
HWRPB RAM range for external image storage. Kernel segments that overlap the
loader or downloaded images are rejected. This is not a general relocating
Alpha loader. The low-memory Linux segments and the EV6+ SRM/PAL environment
used by the current Gentoo image are the qualified combination.

HPPA keeps a separate external image heap in confirmed contiguous PAGE0 RAM
on machines with at least 256 MiB. A position-independent, stack-free copy
trampoline avoids executing from the low memory overwritten by Linux.
Linux's initial command line lives in a checked low-memory region. Both the
32-bit and 64-bit Linux virtual address windows are translated using PALO's
low-physical-memory convention. The client and PDC calls remain narrow even
when the guest OS kernel switches to wide mode on PA-RISC 2.0.

SPARC64 uses physical-bypass ASIs for the Sabre PCI configuration and Intel
MMIO. The host model and register address must match. Direct DMA is enabled
only while the client owns the adapter; the original IOMMU control is
restored before kernel entry. The Linux adapter uses HdrS 3.01 parameters,
reserves its ELF destination and keeps the initramfs in low RAM. The initial
high-initramfs experiment reached userspace but emitted RSS-counter bugs;
the low-memory version is the qualified candidate. SPARC32 uses its own
polled Am7990 driver, with OpenPROM/OpenBIOS DMA allocation/mapping and an
8 KiB buffer bounded by compile-time ring-alignment and size assertions.
It does not turn OpenBIOS into a firmware network bootloader.

The OS proof uses real HTTP downloads of official Gentoo ELF and initramfs,
then requires an executed guest command and a marker on its own output line.
The marker in an echoed command is insufficient. The live root filesystem is
provided by an attached **read-only official ISO**, with writes in guest RAM.
These results qualify the native kernel entry, not a complete network root
profile or a disk installation. No host disk is attached to these guests.
For the G5 kernel without a built-in serial console, the test records a
framebuffer dump, OCR text and keyboard-entered command output.

Example (use an architecture-compatible ELF and a separately available root):

```ipxe
kernel http://server/kernel console=... <root arguments>
initrd --name initrd http://server/initramfs
boot
```

Reproduce with `tools/test-native-linux.py --help`. The recorded command JSON,
SHA256 values, serial logs and (where used) framebuffer proof are under
`/data/netboot/tests/ipxe-fork/r5-*`. SPARC32 Linux entry, Solaris/HP-UX/OpenVMS
entry, real SRM BOOTP, general OpenBoot bootstrap containers, physical hardware
and complete native network-root profiles remain separate milestones.

## HTTP live roots and Sun4m OS entry (r6)

The native Linux fixtures now boot without an attached ISO. A small appended
initramfs member supplies `netboot-init` and a pinned root URL/SHA256, using
the official BusyBox and kernel modules. The prefix obtains DHCP, downloads
SquashFS into guest RAM, verifies its digest, and enters the original init.
The original compressed initramfs remains the first archive member. Kernel
code and SquashFS bytes are unchanged; Alpha's official gzip kernel is
uncompressed before handing its ELF bytes to the native loader.

The genkernel branch needs one path normalisation in a temporary RAM copy of
`/init`: after recognising an already present absolute loop image, it must
strip the directory from `LOOP` before joining it with `CDROOT_PATH` again.
Dracut uses its existing local live-image path. Its profile includes `cdroot`
so that Gentoo configures the live console. Missing network/display modules
and dependencies are copied unchanged from the matching official SquashFS;
no module from a different kernel release is substituted. A missing service
name database is supplied for dynamically linked BusyBox wget.

`tools/wrap-netboot-initrd.py`, `tools/netboot-modules.py` and
`tools/netboot-init` reproduce the prefix. `tools/test-native-linux.py`
checks either a temporary HTTP fixture or an actual published profile.
`--reject-root` proves that a wrong root digest prevents official init entry.
There is no writable installation disk in these tests. The existing PC disk
installation results remain separate from this native live-system milestone.

IA64 chains a GRUB 2.12 standalone EFI image fetched through iPXE. Its memory
filesystem contains the official kernel and augmented initramfs; the live
root comes from HTTP. The experimental IA64 emulator now reaches the real
Gentoo IA64 shell. `tools/test-ia64.py linux` reproduces this path. Its
firmware FAT entry is still a bootstrap fixture, not a firmware PXE proof.

Sun4m now has a bounded ELF32 adapter for Linux HdrS 2.x and canonical NetBSD
identity notes. It passes the original OpenPROM vector and uses a stack-free
copy trampoline outside the overwritten low iPXE image. Destination ranges
must fit the original low mapping. Linux initrd destinations are selected
from the current firmware `available` property, with a checked v0 fallback.
Firmware node handles may have their high bit set and are not signed-positive
integers. The NetBSD ELF note reader has 162 additional portable assertions,
including unaligned tables, truncated prefixes and overflowing ranges.

The official NetBSD 11 SPARC microroot reaches a real RAM shell through iPXE
HTTP on SS-5. This minimal first-stage system lacks ordinary utilities such
as uname and mount and requests additional installer tools; it is not the
complete sysinst distribution. Its successful proof executes `/bin/sh -c
true` after entering the RAM shell. The vintage Debian SPARC32 Linux 2.4.27
kernel reaches its Zilog driver both from this adapter and from a separate
direct QEMU baseline, then stalls there. That is not a completed Linux boot.
Modern large SPARC32 Linux kernels and other OpenPROM models need further
loader/mapping work and are not qualified by this small-kernel fixture.

SPARC32's four-entry LANCE receive ring cannot absorb a 2 MiB TCP burst.
Its platform receive window is capped at 4 KiB, avoiding repeated packet
losses during multi-megabyte transfers. Other architectures retain their
existing window size. The eight native targets now each pass 13,468
assertions; x86-64/Linux passes 13,588.

Published private-LAN live profiles are under
`/data/ipxe/netboot/media/gentoo-native-r6/`; the persistent menu renderer
exposes their architecture/model choices. ISO signatures and SHA512 are
rechecked before extraction by `/home/wynton/netboot/gentoo_native_publish.py`.
The model-dependent first firmware bootstrap, supported physical NICs,
low-memory cases and real hardware still need their own qualification.
The Intel PCI NIC fixtures do not implement the onboard HP 712 LAN,
PowerMac Sungem or Sun HME drivers inside iPXE. Solaris/HP-UX/OpenVMS and
Mezereon transitions through these new clients are separate OS adapters;
the existing firmware-native boot profiles are retained.
