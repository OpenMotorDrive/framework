SECTIONS
{
    .param1(NOLOAD) : {
    } >param1
    .param2(NOLOAD) : {
    } >param2
}

PROVIDE(_app_bl_shared_sec = ORIGIN(app_bl_shared));

PROVIDE(_bl_flash_sec = ORIGIN(bl));
PROVIDE(_bl_flash_sec_end = ORIGIN(bl)+LENGTH(bl));

PROVIDE(_app_flash_sec = ORIGIN(app));
PROVIDE(_app_flash_sec_end = ORIGIN(app)+LENGTH(app));

PROVIDE(_param1_flash_sec = ORIGIN(param1));
PROVIDE(_param1_flash_sec_end = ORIGIN(param1)+LENGTH(param1));

PROVIDE(_param2_flash_sec = ORIGIN(param2));
PROVIDE(_param2_flash_sec_end = ORIGIN(param2)+LENGTH(param2));

/* For each data/text section two region are defined, a virtual region
   and a load region (_LMA suffix).*/

/* Flash region to be used for exception vectors.*/
REGION_ALIAS("VECTORS_FLASH", PROGRAM_REGION);
REGION_ALIAS("VECTORS_FLASH_LMA", PROGRAM_REGION);

/* Flash region to be used for constructors and destructors.*/
REGION_ALIAS("XTORS_FLASH", PROGRAM_REGION);
REGION_ALIAS("XTORS_FLASH_LMA", PROGRAM_REGION);

/* Flash region to be used for code text.*/
REGION_ALIAS("TEXT_FLASH", PROGRAM_REGION);
REGION_ALIAS("TEXT_FLASH_LMA", PROGRAM_REGION);

/* Flash region to be used for read only data.*/
REGION_ALIAS("RODATA_FLASH", PROGRAM_REGION);
REGION_ALIAS("RODATA_FLASH_LMA", PROGRAM_REGION);

/* Flash region to be used for various.*/
REGION_ALIAS("VARIOUS_FLASH", PROGRAM_REGION);
REGION_ALIAS("VARIOUS_FLASH_LMA", PROGRAM_REGION);

/* Flash region to be used for RAM(n) initialization data.*/
REGION_ALIAS("RAM_INIT_FLASH_LMA", PROGRAM_REGION);

/* RAM region to be used for Main stack. This stack accommodates the processing
   of all exceptions and interrupts.*/
REGION_ALIAS("MAIN_STACK_RAM", ram);

/* RAM region to be used for the process stack. This is the stack used by
   the main() function.*/
REGION_ALIAS("PROCESS_STACK_RAM", ram);

/* RAM region to be used for data segment.*/
REGION_ALIAS("DATA_RAM", ram);
REGION_ALIAS("DATA_RAM_LMA", PROGRAM_REGION);

/* RAM region to be used for BSS segment.*/
REGION_ALIAS("BSS_RAM", ram);

/* RAM region to be used for the default heap.*/
REGION_ALIAS("HEAP_RAM", ram);

REGION_ALIAS("ram0", ram)

/* Generic rules inclusion.*/
INCLUDE rules.ld
