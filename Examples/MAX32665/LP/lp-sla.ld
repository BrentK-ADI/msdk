/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by 
 * Analog Devices, Inc.),
 * Copyright (C) 2023-2024 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/* Make room for PAL_NVM_DB */
FLASH_MEM_SIZE = 0x100000 - 0x200 - _PAL_NVM_SIZE;
PAL_NVM_ORIGIN = 0x100000 - _PAL_NVM_SIZE + 0x10000000;

/* SPID and SPIX Sections here are maximum possible sizes */
/* If used, they should be adjusted for the external Flash/RAM size */
MEMORY {

    SPIX (rx)  : ORIGIN = 0x08000000,          LENGTH = 128M
    HEADER (rx): ORIGIN = 0x10000000,          LENGTH = 0x200
    FLASH (rx) : ORIGIN = 0x10000200,          LENGTH = FLASH_MEM_SIZE
    PAL_NVM_DB (r)  : ORIGIN = PAL_NVM_ORIGIN, LENGTH = _PAL_NVM_SIZE
    
    SRAM (rwx)      : ORIGIN = 0x20000000,     LENGTH = 64k /* Limit SRAM usage to SRAM0 and SRAM1 to prevent hardfault. */
    SPID (rw)       : ORIGIN = 0x80000000,     LENGTH = 512M
}

/* Added Oct 9, 2018 to go to correct reset vector. */
ENTRY(Reset_Handler)
PROVIDE( _start_SWAP = (((Reset_Handler) >> 24) | (((Reset_Handler) & 0x00FF0000) >> 8) | (((Reset_Handler) & 0x0000FF00) << 8) | ((Reset_Handler) << 24)));
PROVIDE_HIDDEN( _SLA_Size = _endimage - __end_header );
PROVIDE( _SLA_Size_SWAP = (((_SLA_Size) >> 24) | (((_SLA_Size) & 0x00FF0000) >> 8) | (((_SLA_Size) & 0x0000FF00) << 8) | ((_SLA_Size) << 24)));

/* Sections Definitions */
SECTIONS {
     .sb_sla_header : ALIGN(4)
    {
       FILL(0xFF)
        KEEP(*(.sb_sla_header)) /* Header for ROM code */ 
         __end_header = . ;
        . = ALIGN(512);
    } > HEADER

    .text :
    {
        _text = .;
        KEEP(*(.isr_vector))
        *(.text*)    /* program code */
        *(.rodata*)  /* read-only data: "const" */
        
        KEEP(*(.init))
        KEEP(*(.fini))

        /* C++ Exception handling */
        KEEP(*(.eh_frame*))
        _etext = .;
    } > FLASH
    
    /* This section will keep the SPIX data until loaded into the external device */
    /* Upon initialization of SPIX (user code needs to do this) */
    .xip_section :
    {
        KEEP(*(.xip_section*))
    } > SPIX AT>FLASH

    __load_start_xip = LOADADDR(.xip_section);
    __load_length_xip = SIZEOF(.xip_section);

    /* it's used for C++ exception handling      */
    /* we need to keep this to avoid overlapping */
    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .data :
    {
        _data = ALIGN(., 4);
        *(.data*)           /*read-write initialized data: initialized global variable*/
        *(.spix_config*)    /* SPIX configuration functions need to be run from SRAM */
        *(.flashprog*)      /* Flash program */
        

        /* These array sections are used by __libc_init_array to call static C++ constructors */
        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        _edata = ALIGN(., 4);
    } > SRAM AT>FLASH
    __load_data = LOADADDR(.data);
    _enddata = LOADADDR(.data)+SIZEOF(.data);

    .sb_sla_trailer : AT(_enddata)
    {
        KEEP(*(.sb_sla_trailer))
        /* Align image with 16 byte boundary to conform to flash encryption block size. */
        FILL(0xDEADC0DE);
        /* NOTE: The FILL and ALIGN will not work unless something is written to the section.  So, we use LONG. */
        LONG(0xDEADC0DE);
        . = ALIGN(16);
    }  > FLASH
    _endimage = LOADADDR(.sb_sla_trailer)+SIZEOF(.sb_sla_trailer);
    .sig :
    {
        KEEP(*(.sig))
        LONG(0xDEADBEEF);

    }  > FLASH

    .pal_nvm_db :
    {
    /* Align to the sector size */
    . = ALIGN(0x2000);
    __pal_nvm_db_start__ = .;
    . = . + _PAL_NVM_SIZE;
    __pal_nvm_db_end__ = .;
    } > PAL_NVM_DB

    .bss :
    {
        . = ALIGN(4);
        _bss = .;
        *(.bss*)     /*read-write zero initialized data: uninitialzed global variable*/
        *(COMMON)
        _ebss = ALIGN(., 4);
    } > SRAM

    /* Setup the stack for Core 1, it will only be used if the user code
     * includes a definition of Stack_Size_Core1, which defines the space
     * reserved above the main core's stack for core 1's stack */

    __StackTop_Core1 = ORIGIN(SRAM) + LENGTH(SRAM);
    __StackLimit_Core1 = DEFINED(Stack_Size_Core1) ? __StackTop_Core1 - Stack_Size_Core1 : __StackTop_Core1;

    /* Set stack top to end of RAM, and stack limit move down by Stack_Size. 
     * If core 1 is used, set the stack to the bottom of Core 1's stack region */

    __StackTop = DEFINED(Stack_Size_Core1) ? __StackLimit_Core1 : ORIGIN(SRAM) + LENGTH(SRAM);
    __StackLimit = __StackTop - Stack_Size;

    .heap (COPY):
    {
        . = ALIGN(4);
        *(.heap*)
        __HeapLimit = ABSOLUTE(__StackLimit);
    } > SRAM

    PROVIDE(__stack = __StackTop);

    /* Check if data + heap + stack(s) exceeds RAM limit */
    ASSERT(__StackLimit >= _ebss, "region RAM overflowed with stack")
}
