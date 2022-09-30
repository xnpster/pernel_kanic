/* Simple linker script for JOS user-level programs.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* Load programs at this address: "." means the current address */
    . = #ADDRESS#;

    .text : {
        *(.text .stub .text.* .gnu.linkonce.t.*)

        . = ALIGN(8);

        PROVIDE(etext = .); /* Define the 'etext' symbol to this value */

        *(.rodata .rodata.* .gnu.linkonce.r.* .data.rel.ro.local)
    }


    /* Adjust the address for the data segment to the next page */
    . = ALIGN(0x1000);

    .data : {
        *(.data .data.rel.local .got .got.plt)
    }

    PROVIDE(edata = .);

    .bss : {
        *(.bss)
    }

    PROVIDE(end = .);


    /DISCARD/ : {
        *(.interp .eh_frame .note.GNU-stack .comment)
    }
}
