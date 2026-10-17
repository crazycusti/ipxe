# Experimental iPXE multiarchitecture fork

This fork adds native IEEE 1275 clients for SPARC64 and 32-bit PowerPC.
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
| SPARC32 / JavaStation | Separate CPU port required | Pending | Pending | Pending |
| HPPA / PDC | Separate firmware adapter and CPU port required | Pending | Pending | Pending |
| PPC64 / PPC64LE ELF clients | Separate ABI/entry work required | Pending | A PPC32 SLOF client already runs on a POWER9 machine | Pending |

There is no physical-hardware qualification. These binaries have not replaced
the operational PC, JavaStation, HPPA or Solaris boot files. A running iPXE
menu does not qualify Linux, Solaris, HP-UX or Mezereon kernel loading.

The SPARC64 bootstrap currently uses virtual/physical address `0x40004000`
to match the QEMU OpenBIOS direct-kernel entry. Its QEMU fixture therefore
uses 2 GiB RAM. Relocation, a suitable general OpenBoot network container and
lower-memory machines need further work before physical deployment.

## Structure

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
statically linked from the matching cross-compiler's `libgcc`; no libc or
other operating-system runtime is linked into the native image.

The initial firmware platform enables HTTP/TFTP. HTTPS is disabled by default
until qualified entropy and wall-clock providers exist. The cryptographic
self-tests still exercise the underlying algorithms and wire formats.

## Build

On an Ubuntu build host, install the matching cross compiler and QEMU:

```sh
sudo apt-get install gcc-sparc64-linux-gnu gcc-powerpc-linux-gnu qemu-system-sparc qemu-system-ppc
make -C src CROSS=sparc64-linux-gnu- bin-sparc64-openfirmware/ipxe.of
make -C src CROSS=powerpc-linux-gnu- bin-powerpc-openfirmware/ipxe.of
```

Use explicit target names. The existing x86 default and operational PC
loaders are independent of these new targets. `none.of` omits the network
adapter; `tests.of` executes upstream's native self-test image. A normal iPXE
script can be embedded using `EMBED=/absolute/path/to/script.ipxe`.

## Reproducible tests

Each command requires a fresh output directory:

```sh
python3 tools/test-ieee1275.py sparc64 selftests /tmp/ipxe-sparc64-tests
python3 tools/test-ieee1275.py sparc64 menu /tmp/ipxe-sparc64-menu
python3 tools/test-ieee1275.py powerpc selftests /tmp/ipxe-ppc-tests
python3 tools/test-ieee1275.py powerpc network /tmp/ipxe-ppc-network
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

SPARC32 needs its own entry/ABI and cannot use the SPARC64 image. HPPA uses
PDC/IODC and a LIF/PALO-compatible entry; it does not use the IEEE 1275 backend.
Native PPC64 and little-endian kernel entry need separate qualification even
when the initial firmware client is PPC32.

The first follow-up milestones are an appropriate SPARC network container,
firmware packet I/O on real OpenBoot, low-memory/relocation support and
architecture-specific executable handoff. Native PCI/DMA backends are a
separate alternative for firmware without packet I/O. Solaris/HP-UX retain
their native installation protocols and media requirements.

Changes are maintained in this fork. Upstream submissions follow the human
ownership and review requirements in `AGENTS.md` and `CONTRIBUTING.md`.
