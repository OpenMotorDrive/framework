/* Enforce emmition of the vector table. */
EXTERN (vector_table)

/* Define the entry point of the output file. */
ENTRY(reset_handler)

/* Define sections. */
SECTIONS
{
    .text : {
        *(.vectors)	/* Vector table */
        KEEP(*(.app_descriptor))
        *(.text*)	/* Program code */
        . = ALIGN(4);
        *(.rodata*)	/* Read-only data */
        . = ALIGN(4);
    } >PROGRAM_REGION

    .bl (NOLOAD) : {
    } >bl

    .app (NOLOAD) : {
    } >app

    .params(NOLOAD) : {
    } >params

    .app_bl_shared (NOLOAD) : {
    } >app_bl_shared

    /* C++ Static constructors/destructors, also used for __attribute__
        * ((constructor)) and the likes */
    .preinit_array : {
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
    } >PROGRAM_REGION
    .init_array : {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
    } >PROGRAM_REGION
    .fini_array : {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;
    } >PROGRAM_REGION

    /*
        * Another section used by C++ stuff, appears when using newlib with
        * 64bit (long long) printf support
        */
    .ARM.extab : {
        *(.ARM.extab*)
    } >PROGRAM_REGION
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >PROGRAM_REGION

    . = ALIGN(4);
    _etext = .;

    .data : {
        _data = .;
        *(.data*)	/* Read-write initialized data */
        . = ALIGN(4);
        _edata = .;
    } >ram AT >PROGRAM_REGION
    _data_loadaddr = LOADADDR(.data);

    .bss : {
        *(.bss*)	/* Read-write zero initialized data */
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >ram

    /*
        * The .eh_frame section appears to be used for C++ exception handling.
        * You may need to fix this if you're using C++.
        */
    /DISCARD/ : { *(.eh_frame) }

    . = ALIGN(4);
    end = .;
}

PROVIDE(_stack = ORIGIN(ram) + LENGTH(ram) - 8);
PROVIDE(_app_bl_shared_sec = ORIGIN(app_bl_shared));

