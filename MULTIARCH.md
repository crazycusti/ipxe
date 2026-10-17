# Experimental iPXE multiarchitecture fork

This fork adds native SPARC32/OpenPROM, SPARC64 and PPC32/IEEE 1275,
and narrow HPPA/PDC clients.
It is intended as a foundation for a shared netboot environment and future
Mezereon loading. The upstream tree and its licences remain intact.

Development branch: `feature/ieee1275-multiarch`.
Initial upstream base: `c51ca76` (the full revision is available with
`git merge-base HEAD upstream/master`).

## Verified scope

| Target | Native execution | Self-tests | Firmware/network/menu | OS handoff |
|---|---|---|---|---|
| SPARC64 / OpenBIOS sun4u | Yes, ELF bootstrap and real iPXE core | 12,240 passed | Local iPXE menu passed; stock OpenBIOS lacks the packet-I/O device used by the firmware network adapter | Not implemented or qualified |
| PowerPC 32-bit client / SLOF pseries POWER9 | Yes; a native PPC32 client, not a PPC64 executable | 12,240 passed | Firmware DHCP/TFTP loads iPXE; iPXE DHCP, HTTP, SHA256 and downloaded menu passed | Not implemented or qualified |
| PowerMac / OpenBIOS | Build available | Not qualified on this firmware | No packet-I/O qualification | Not qualified |
| SPARC32 / OpenBIOS SS-5 (V8) | Yes, V8 ELF bootstrap and OpenPROM v2/v3 adapter | 12,240 passed | Serial menu passed; firmware network path is not qualified | Not implemented or qualified |
| HPPA / PDC B160L | Yes, narrow PA-RISC 1.1 | 12,240 passed | Firmware SCSI/PALO boot, PDC console/menu, native Dino/Intel DHCP, HTTP/SHA256 and downloaded menu passed | Not implemented or qualified |
| PPC64 / PPC64LE ELF clients | Separate ABI/entry work required | Pending | A PPC32 SLOF client already runs on a POWER9 machine | Pending |

There is no physical-hardware qualification. These binaries have not replaced
the operational PC, JavaStation, HPPA or Solaris boot files. A running iPXE
menu does not qualify Linux, Solaris, HP-UX or Mezereon kernel loading.

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
