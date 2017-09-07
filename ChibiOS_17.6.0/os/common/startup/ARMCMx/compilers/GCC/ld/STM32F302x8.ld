/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
 * STM32F302x8 memory setup.
 */

 MEMORY
{
    bl (rx) :             ORIGIN = 0x08000000,            LENGTH = 10K
    app (rx) :            ORIGIN = 0x08000000+10K,        LENGTH = 64K-10K-4K
    params (rx) :         ORIGIN = 0x08000000+60K,        LENGTH = 4K
    ram (rwx) :           ORIGIN = 0x20000000,            LENGTH = 16K-256
    app_bl_shared (rwx) : ORIGIN = 0x20000000+(16K-256),  LENGTH = 256
}

REGION_ALIAS("PROGCODE", app)

/* For each data/text section two region are defined, a virtual region
   and a load region (_LMA suffix).*/

/* Flash region to be used for exception vectors.*/
REGION_ALIAS("VECTORS_FLASH", PROGCODE);
REGION_ALIAS("VECTORS_FLASH_LMA", PROGCODE);

/* Flash region to be used for constructors and destructors.*/
REGION_ALIAS("XTORS_FLASH", PROGCODE);
REGION_ALIAS("XTORS_FLASH_LMA", PROGCODE);

/* Flash region to be used for code text.*/
REGION_ALIAS("TEXT_FLASH", PROGCODE);
REGION_ALIAS("TEXT_FLASH_LMA", PROGCODE);

/* Flash region to be used for read only data.*/
REGION_ALIAS("RODATA_FLASH", PROGCODE);
REGION_ALIAS("RODATA_FLASH_LMA", PROGCODE);

/* Flash region to be used for various.*/
REGION_ALIAS("VARIOUS_FLASH", PROGCODE);
REGION_ALIAS("VARIOUS_FLASH_LMA", PROGCODE);

/* Flash region to be used for RAM(n) initialization data.*/
REGION_ALIAS("RAM_INIT_FLASH_LMA", PROGCODE);

/* RAM region to be used for Main stack. This stack accommodates the processing
   of all exceptions and interrupts.*/
REGION_ALIAS("MAIN_STACK_RAM", ram);

/* RAM region to be used for the process stack. This is the stack used by
   the main() function.*/
REGION_ALIAS("PROCESS_STACK_RAM", ram);

/* RAM region to be used for data segment.*/
REGION_ALIAS("DATA_RAM", ram);
REGION_ALIAS("DATA_RAM_LMA", PROGCODE);

/* RAM region to be used for BSS segment.*/
REGION_ALIAS("BSS_RAM", ram);

/* RAM region to be used for the default heap.*/
REGION_ALIAS("HEAP_RAM", ram);

/* Generic rules inclusion.*/
INCLUDE rules.ld
