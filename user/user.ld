/* Simple linker script for JOS user-level programs.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
	/* Load programs at this address: "." means the current address */
	. = 0x800000;

	.text : ALIGN(0x1000) {
		__text_start = .;
		*(.text .stub .text.* .gnu.linkonce.t.*)
		__text_end = .;

		. = ALIGN(0x1000);
		PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */
	}

	.rodata : ALIGN(0x1000) {
		__rodata_start = .;
		*(.rodata .rodata.* .gnu.linkonce.r.* .data.rel.ro.local)
		. = ALIGN(0x1000);
		__rodata_end = .;
	}

	.data : ALIGN(0x1000) {
		__data_start = .;
		*(.data .data.*)
		__data_end = .;

		__ctors_start = .;
		KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP(*(.init_array .ctors))
		__ctors_end = .;
		. = ALIGN(8);

		__dtors_start = .;
		KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP(*(.fini_array .dtors))
		__dtors_end = .;
		. = ALIGN(8);

		*(.got*)

		. = ALIGN(0x1000);
		PROVIDE(edata = .);
	}


	.bss : ALIGN(0x1000) {
		__bss_start = .;
		*(.bss)
		*(COMMON)
		/* Ensure page-aligned segment size */
		. = ALIGN(0x1000);
		__bss_end = .;
	}

	PROVIDE(end = .);

	/DISCARD/ : {
		*(.interp .eh_frame .note.* .comment)
	}
}
