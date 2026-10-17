/* reloc_ia64.S - position independent IA-64 ELF shared object relocator
   Copyright (C) 1999 Hewlett-Packard Co.
	Contributed by David Mosberger <davidm@hpl.hp.com>.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials
      provided with the distribution.
    * Neither the name of Hewlett-Packard Co. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
    BE LIABLE FOR ANYDIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
    OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
    TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
    THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/

/*
 * This is written in assembly because the entire code needs to be position
 * independent.  Note that the compiler does not generate code that's position
 * independent by itself because it relies on the global offset table being
 * relocated.
 */
	FILE_LICENCE ( BSD3 );
	.text
	.psr abi64
	.psr lsb
	.lsb

/*
 * This constant determines how many R_IA64_FPTR64LSB relocations we
 * can deal with.  If you get EFI_BUFFER_TOO_SMALL errors, you may
 * need to increase this number.
 */
#define MAX_FUNCTION_DESCRIPTORS	8192

#define ST_VALUE_OFF	8		/* offset of st_value in elf sym */

#define EFI_SUCCESS		0
#define EFI_LOAD_ERROR		0x8000000000000001
#define EFI_BUFFER_TOO_SMALL	0x8000000000000005

#define DT_NULL		0		/* Marks end of dynamic section */
#define DT_RELA		7		/* Address of Rela relocs */
#define DT_RELASZ	8		/* Total size of Rela relocs */
#define DT_RELAENT	9		/* Size of one Rela reloc */
#define DT_SYMTAB	6		/* Address of symbol table */
#define DT_SYMENT	11		/* Size of one symbol table entry */

#define R_IA64_NONE		0
#define R_IA64_REL64MSB		0x6e
#define R_IA64_REL64LSB		0x6f
#define R_IA64_DIR64MSB		0x26
#define R_IA64_DIR64LSB		0x27
#define R_IA64_FPTR64MSB	0x46
#define R_IA64_FPTR64LSB	0x47

#define	ldbase	in0	/* load address (address of .text) */
#define	dyn	in1	/* address of _DYNAMIC */

#define d_tag	r16
#define d_val	r17
#define rela	r18
#define relasz	r19
#define relaent	r20
#define addr	r21
#define r_info	r22
#define r_offset r23
#define r_addend r24
#define r_type	r25
#define r_sym	r25	/* alias of r_type ! */
#define fptr	r26
#define fptr_limit r27
#define symtab	f8
#define syment	f9
#define ftmp	f10

#define	target	r16
#define val	r17

#define NLOC	0

#define Pnull		p6
#define Prela		p7
#define Prelasz		p8
#define Prelaent	p9
#define Psymtab		p10
#define Psyment		p11

#define Pnone		p6
#define Prel		p7
#define Pfptr		p8

#define Pmore		p6

#define Poom		p6	/* out-of-memory */

	.global _relocate
	.proc _relocate
_relocate:
	alloc r2=ar.pfs,2,0,0,0
	movl	fptr = @gprel(fptr_mem_base)

	add	fptr = fptr, gp
	movl	fptr_limit = @gprel(fptr_mem_limit)

	add	fptr_limit = fptr_limit, gp

search_dynamic:
	ld8	d_tag = [dyn],8

	ld8	d_val = [dyn],8
	cmp.eq	Pnull,p0 = DT_NULL,d_tag
(Pnull)	br.cond.sptk.few apply_relocs
	cmp.eq	Prela,p0 = DT_RELA,d_tag
	cmp.eq	Prelasz,p0 = DT_RELASZ,d_tag
	cmp.eq	Psymtab,p0 = DT_SYMTAB,d_tag
	cmp.eq	Psyment,p0 = DT_SYMENT,d_tag
	cmp.eq	Prelaent,p0 = DT_RELAENT,d_tag

(Prela)	add rela = d_val, ldbase
(Prelasz) mov relasz = d_val
(Prelaent) mov relaent = d_val
(Psymtab) add val = d_val, ldbase

(Psyment) setf.sig syment = d_val

(Psymtab) setf.sig symtab = val
	br.sptk.few search_dynamic

apply_loop:
	ld8	r_offset = [rela]
	add	addr = 8,rela
	sub	relasz = relasz,relaent


	ld8	r_info = [addr],8

	ld8	r_addend = [addr]
	add	target = ldbase, r_offset

	add	rela = rela,relaent
	extr.u	r_type = r_info, 0, 32

	cmp.eq	Pnone,p0 = R_IA64_NONE,r_type
	cmp.eq	Prel,p0 = R_IA64_REL64LSB,r_type
	cmp.eq	Pfptr,p0 = R_IA64_FPTR64LSB,r_type
(Prel)	br.cond.sptk.few apply_REL64

	cmp.eq	Prel,p0 = R_IA64_DIR64LSB,r_type // treat DIR64 just like REL64

(Pnone)	br.cond.sptk.few apply_relocs
(Prel)	br.cond.sptk.few apply_REL64
(Pfptr)	br.cond.sptk.few apply_FPTR64

	movl	r8 = EFI_LOAD_ERROR
	br.ret.sptk.few rp

apply_relocs:
	cmp.ltu	Pmore,p0=0,relasz
(Pmore)	br.cond.sptk.few apply_loop

	mov	r8 = EFI_SUCCESS
	br.ret.sptk.few rp

apply_REL64:
	ld8 val = [target]

	add val = val,ldbase

	st8 [target] = val
	br.cond.sptk.few apply_relocs

	// Canonicalize descriptors by relocated function address. iPXE uses
	// function pointers as interface operation IDs; allocating one per
	// relocation would make equal functions compare unequal.
apply_FPTR64:
	extr.u r_sym = r_info,32,32
	setf.sig ftmp = r_sym
	xma.lu ftmp = ftmp,syment,symtab
	getf.sig addr = ftmp
	add addr = ST_VALUE_OFF,addr
	ld8 val = [addr]
	add r29 = val,ldbase
	add r29 = r29,r_addend
	movl r28 = @gprel(fptr_mem_base)
	add r28 = r28,gp
find_fptr:
	cmp.geu p6,p0 = r28,fptr
(p6) br.cond.sptk.few new_fptr
	ld8 r30 = [r28]
	cmp.eq p6,p0 = r30,r29
(p6) br.cond.sptk.few found_fptr
	add r28 = 16,r28
	br.cond.sptk.few find_fptr
new_fptr:
	cmp.geu p6,p0 = fptr,fptr_limit
(p6) br.cond.spnt.few fptr_full
	st8 [fptr] = r29,8
	st8 [fptr] = gp,8
found_fptr:
	st8 [target] = r28
	br.cond.sptk.few apply_relocs
fptr_full:
	movl r8 = EFI_BUFFER_TOO_SMALL
	br.ret.sptk.few rp

	.endp _relocate

	.data
	.align 16
fptr_mem_base:
	.space  MAX_FUNCTION_DESCRIPTORS*16
fptr_mem_limit:

#if defined(__ELF__) && defined(__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
