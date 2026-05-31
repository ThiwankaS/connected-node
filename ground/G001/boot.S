# boot.S
# Pure Xtensa Assembly bootstrapping vector

    # Tells the linker to place the following code inside a section named ".entry.text", 
    # and flags it as allocatable ("a") and executable ("x").
    .section .entry.text, "ax"
    # Declares the symbol 'call_start_cpu0' as globally visible, allowing the linker 
    # to see it as the explicit entry point matching linker script configuration.
    .global call_start_cpu0
    # Tells the assembler that 'call_start_cpu0' is a functional block of executable code.
    .type call_start_cpu0, @function
    # Forces the next instruction or data memory structure to be aligned to a strict 4-byte boundary.
    .align 4
    # Directs the assembler to place the "literal pools" (constants used by instructions like movi/l32r) 
    # directly right here in memory to prevent dangerous Xtensa relocation faults.
    .literal_position
# Defines the starting label where execution jumps immediately after the internal hardware ROM stage finishes.
call_start_cpu0:
    # Clear Xtensa windowed registers (Standard ABI setup)
    movi    a0, 0               # Loads the immediate integer value 0 into CPU register a0.
    wsr     a0, windowstart     # Writes the value of register a0 (0) into the special 'windowstart' state register to reset the register
    movi    a0, 1               # Loads the immediate integer value 1 into CPU register a0.
    wsr     a0, windowbase      # Writes the value of register a0 (1) into the special 'windowbase' state register
    rsync                       # Forces an execution sync; 
                                # freezes the pipeline until all preceding special register updates (wsr) are physically complete.

    # Assign the stack pointer to top of DRAM
    # Resolves the external linker address '_stack_top' and loads that 32-bit memory location directly into the Stack Pointer register (sp/a1).
    movi    sp, _stack_top

    # Zero out the BSS section manually
    # Loads the start address of uninitialized static data variables ('_sbss' from the linker script) into register a2
    movi    a2, _sbss
    # Loads the end address of uninitialized static data variables ('_ebss' from the linker script) into register a3
    movi    a3, _ebss
    # Loads a constant value of 0 into register a4; this will be used to clear out the target memory.
    movi    a4, 0
# Internal loop label used to systematically scrub the BSS memory clear.
.clear_bss_loop:
    # Conditional Branch: If the current pointer address in a2 is Greater Than or Equal Unsigned to the end address in a3, jump out to '.bss_done'.
    bgeu    a2, a3, .bss_done
    # Store 32-bit Immediate
    # Writes the contents of register a4 (which is 0) into the RAM address pointed to by register a2 with an offset of 0 bytes.
    s32i    a4, a2, 0
    # Adds a value of 4 to register a2, incrementing our pointer to the next 32-bit (4-byte) memory address word slot.
    addi    a2, a2, 4
    # Unconditional Jump: Loops back up to the '.clear_bss_loop' label to inspect and wipe the next address space.
    j       .clear_bss_loop
# Label reached once the memory clear conditions have been fully met.
.bss_done:
    # Jump safely into the pure bare-metal C++ main loop
    # Executes a raw, non-windowed functional call directly to our C++ runtime entry function 'bare_metal_main'.
    call0   bare_metal_main
# Emergency containment label. If 'bare_metal_main' ever returns or crashes out, execution traps here.
.loop_freeze:
    # Trap loop instruction: continually branches to itself forever to prevent the CPU from running off into random garbage memory.
    j       .loop_freeze
