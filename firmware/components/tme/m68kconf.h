/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */
/*
 *                                  MUSASHI
 *                                Version 3.4
 *
 * A portable Motorola M680x0 processor emulation engine.
 * Copyright 1998-2001 Karl Stenerud.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */



#ifndef M68KCONF__HEADER
#define M68KCONF__HEADER


/* Configuration switches.
 * Use OPT_SPECIFY_HANDLER for configuration options that allow callbacks.
 * OPT_SPECIFY_HANDLER causes the core to link directly to the function
 * or macro you specify, rather than using callback functions whose pointer
 * must be passed in using m68k_set_xxx_callback().
 */
#define OPT_OFF             0
#define OPT_ON              1
#define OPT_SPECIFY_HANDLER 2


/* ======================================================================== */
/* ============================== MAME STUFF ============================== */
/* ======================================================================== */

/* If you're compiling this for MAME, only change M68K_COMPILE_FOR_MAME
 * to OPT_ON and use m68kmame.h to configure the 68k core.
 */
#ifndef M68K_COMPILE_FOR_MAME
#define M68K_COMPILE_FOR_MAME      OPT_OFF
#endif /* M68K_COMPILE_FOR_MAME */


#if M68K_COMPILE_FOR_MAME == OPT_OFF


/* ======================================================================== */
/* ============================= CONFIGURATION ============================ */
/* ======================================================================== */

/* Turn ON if you want to use the following M68K variants */
#define M68K_EMULATE_010            OPT_OFF
#define M68K_EMULATE_EC020          OPT_OFF
#define M68K_EMULATE_020            OPT_OFF


/* If ON, the CPU will call m68k_read_immediate_xx() for immediate addressing
 * and m68k_read_pcrelative_xx() for PC-relative addressing.
 * If off, all read requests from the CPU will be redirected to m68k_read_xx()
 */
#define M68K_SEPARATE_READS         OPT_ON
//#define M68K_SEPARATE_READS         OPT_OFF


/* If ON, the CPU will call m68k_write_32_pd() when it executes move.l with a
 * predecrement destination EA mode instead of m68k_write_32().
 * To simulate real 68k behavior, m68k_write_32_pd() must first write the high
 * word to [address+2], and then write the low word to [address].
 */
#define M68K_SIMULATE_PD_WRITES     OPT_OFF

/* If ON, CPU will call the interrupt acknowledge callback when it services an
 * interrupt.
 * If off, all interrupts will be autovectored and all interrupt requests will
 * auto-clear when the interrupt is serviced.
 */
#define M68K_EMULATE_INT_ACK        OPT_OFF
#define M68K_INT_ACK_CALLBACK(A)    m68k_int_ack(A)


/* If ON, CPU will call the breakpoint acknowledge callback when it encounters
 * a breakpoint instruction and it is running a 68010+.
 */
#define M68K_EMULATE_BKPT_ACK       OPT_OFF
#define M68K_BKPT_ACK_CALLBACK()    your_bkpt_ack_handler_function()


/* If ON, the CPU will monitor the trace flags and take trace exceptions
 */
#define M68K_EMULATE_TRACE          OPT_OFF


/* If ON, CPU will call the output reset callback when it encounters a reset
 * instruction.
 */
#define M68K_EMULATE_RESET          OPT_OFF
#define M68K_RESET_CALLBACK()       your_reset_handler_function()


/* If ON, CPU will call the set fc callback on every memory access to
 * differentiate between user/supervisor, program/data access like a real
 * 68000 would.  This should be enabled and the callback should be set if you
 * want to properly emulate the m68010 or higher. (moves uses function codes
 * to read/write data from different address spaces)
 */
#define M68K_EMULATE_FC             OPT_OFF
#define M68K_SET_FC_CALLBACK(A)     your_set_fc_handler_function(A)


/* If ON, CPU will call the pc changed callback when it changes the PC by a
 * large value.  This allows host programs to be nicer when it comes to
 * fetching immediate data and instructions on a banked memory system.
 */
#define M68K_MONITOR_PC             OPT_SPECIFY_HANDLER
#define M68K_SET_PC_CALLBACK(A)     m68k_pc_changed_handler_function(A)
void m68k_pc_changed_handler_function(unsigned int addr);

/* If ON, CPU will call the instruction hook callback before every
 * instruction.
 */
//#define M68K_INSTRUCTION_HOOK       OPT_SPECIFY_HANDLER
#define M68K_INSTRUCTION_HOOK       OPT_OFF
#define M68K_INSTRUCTION_CALLBACK() m68k_instruction()
void m68k_instruction();

/* If ON, the CPU will emulate the 4-byte prefetch queue of a real 68000 */
#define M68K_EMULATE_PREFETCH       OPT_OFF


/* If ON, the CPU will generate address error exceptions if it tries to
 * access a word or longword at an odd address.
 * NOTE: This is only emulated properly for 68000 mode.
 */
#define M68K_EMULATE_ADDRESS_ERROR  OPT_OFF


/* Turn ON to enable logging of illegal instruction calls.
 * M68K_LOG_FILEHANDLE must be #defined to a stdio file stream.
 * Turn on M68K_LOG_1010_1111 to log all 1010 and 1111 calls.
 */
#define M68K_LOG_ENABLE             OPT_OFF
#define M68K_LOG_1010_1111          OPT_OFF
#define M68K_LOG_FILEHANDLE         stderr


/* ----------------------------- COMPATIBILITY ---------------------------- */

/* The following options set optimizations that violate the current ANSI
 * standard, but will be compliant under the forthcoming C9X standard.
 */


/* If ON, the enulation core will use 64-bit integers to speed up some
 * operations.
*/
#define M68K_USE_64_BIT  OPT_OFF


/* Set to your compiler's static inline keyword to enable it, or
 * set it to blank to disable it.
 * If you define INLINE in the makefile, it will override this value.
 * NOTE: not enabling inline functions will SEVERELY slow down emulation.
 */
#ifndef INLINE
#define INLINE static __inline__
#endif /* INLINE */

#endif /* M68K_COMPILE_FOR_MAME */


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */


#ifdef HOSTBUILD
	#include <byteswap.h>
#else
	#include "my_byteswap.h"
#endif
#include <stdint.h>

extern unsigned char *m68k_pcbase;

static inline unsigned int m68k_read_immediate_16(unsigned int address) {
	address&=0xFFFFFF;
	uint16_t *p=(uint16_t*)(m68k_pcbase+address);
	return __bswap_16(*p);
}

static inline unsigned int m68k_read_immediate_32(unsigned int address) {
	address&=0xFFFFFF;
	uint32_t *p=(uint32_t*)(m68k_pcbase+address);
	return __bswap_32(*p);
}

static inline unsigned int  m68k_read_pcrelative_8(unsigned int address) {
	address&=0xFFFFFF;
	uint8_t *p=(uint8_t*)(m68k_pcbase+address);
	return *p;
}

static inline unsigned int  m68k_read_pcrelative_16(unsigned int address) {
	address&=0xFFFFFF;
	uint16_t *p=(uint16_t*)(m68k_pcbase+address);
	return __bswap_16(*p);
}

static inline unsigned int  m68k_read_pcrelative_32(unsigned int address) {
	address&=0xFFFFFF;
	uint32_t *p=(uint32_t*)(m68k_pcbase+address);
	return __bswap_32(*p);
}


#endif /* M68KCONF__HEADER */
