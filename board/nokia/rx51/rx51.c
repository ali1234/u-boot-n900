/*
 * (C) Copyright 2010
 * Alistair Buxton <a.j.buxton@gmail.com>
 *
 * Derived from Beagle Board and 3430 SDP code:
 * (C) Copyright 2004-2008
 * Texas Instruments, <www.ti.com>
 *
 * Author :
 *	Sunil Kumar <sunilsaini05@gmail.com>
 *	Shashi Ranjan <shashiranjanmca05@gmail.com>
 *
 *	Richard Woodruff <r-woodruff2@ti.com>
 *	Syed Mohammed Khasim <khasim@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <twl4030.h>
#include <i2c.h>
#include <video_fb.h>
#include <asm/io.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/gpio.h>
#include <asm/mach-types.h>
#include "rx51.h"

GraphicDevice gdev;

extern unsigned int _INIT_LOADADDR;
extern unsigned int _INIT_ATAGADDR;

/*
 * Routine: board_init
 * Description: Early hardware init.
 */
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gpmc_init(); /* in SRAM or SDRAM, finish GPMC */
	/* board id for Linux */
	gd->bd->bi_arch_number = MACH_TYPE_NOKIA_RX51;
	/* boot param addr */
	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100);

	return 0;
}

/*
 * Routine: video_hw_init
 * Description: Set up the GraphicDevice depending on sys_boot.
 */
void *video_hw_init(void)
{
	/* fill in Graphic Device */
	gdev.frameAdrs = 0x8f9c0000;
	gdev.winSizeX = 800;
	gdev.winSizeY = 480;
	gdev.gdfBytesPP = 2;
	gdev.gdfIndex = GDF_16BIT_565RGB;
	memset((void *)gdev.frameAdrs, 0, 0xbb800);
	return (void *) &gdev;
}

/*
 * Routine: misc_init_r
 * Description: Configure board specific parts
 */
int misc_init_r(void)
{
#ifdef CONFIG_CHAINLOADER
	char buf[12];
	printf("Getting NOLO supplied boot parameters...\n");
	sprintf(buf, "%#x", _INIT_LOADADDR);
	setenv("init_loadaddr", buf);
	sprintf(buf, "%#x", _INIT_LOADADDR+0x40000);
	setenv("init_kernaddr", buf);
	sprintf(buf, "%#x", _INIT_ATAGADDR);
	setenv("init_atagaddr", buf);
#endif

	dieid_num_r();

	return 0;
}

/*
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers specific to the
 *		hardware. Many pins need to be moved from protect to primary
 *		mode.
 */
void set_muxconf_regs(void)
{
	MUX_RX51();
}

/*
 * TWL4030 keypad handler for cfb_console
 */

char keymap[] = {
	/* normal */
	'q',  'o',  'p',  ',', '\b',    0,  'a',  's',
	'w',  'd',  'f',  'g',  'h',  'j',  'k',  'l',
	'e',  '.',    0,  '\r',   0,  'z',  'x',  'c',
	'r',  'v',  'b',  'n',  'm',  ' ',    0,    0,
	't',    0,    0,    0,    0,    0,    0,    0,
	'y',    0,    0,    0,    0,    0,    0,    0,
	'u',    0,    0,    0,    0,    0,    0,    0,
	'i',    0,    0,    0,    0,    0,    0,    0,
	/* fn */
	'1',  '9',  '0',  '=', '\b',    0,  '*',  '+',
	'2',  '#',  '-',  '_',  '(',  ')',  '&',  '!',
	'3',  '?',    0, '\r',    0,    0,  '$',    0,
	'4',  '/', '\\',  '"', '\'',  '@',    0,    0,
	'5',    0,    0,    0,    0,    0,    0,    0,
	'6',    0,    0,    0,    0,    0,    0,    0,
	'7',    0,    0,    0,    0,    0,    0,    0,
	'8',    0,    0,    0,    0,    0,    0,    0,
};

u8 keys[8];
u8 old_keys[8] = {0, 0, 0, 0, 0, 0, 0, 0};
#define KEYBUF_SIZE 4
u8 keybuf[KEYBUF_SIZE];
u8 keybuf_head = 0, keybuf_tail = 0;

int rx51_kp_init(void)
{
	int ret = 0;
	u8 ctrl;
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &ctrl,
		TWL4030_KEYPAD_KEYP_CTRL_REG);

	if (!ret) {
		/* turn on keyboard and use hardware scanning */
		ctrl |= TWL4030_KEYPAD_CTRL_KBD_ON;
		ctrl |= TWL4030_KEYPAD_CTRL_SOFT_NRST;
		ctrl |= TWL4030_KEYPAD_CTRL_SOFTMODEN;
		ret |= twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, ctrl,
					TWL4030_KEYPAD_KEYP_CTRL_REG);
		/* enable key event status */
		ret |= twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, 0xfe,
					TWL4030_KEYPAD_KEYP_IMR1);
		/* using the second interrupt event breaks meamo pr1.2 kernel */
		/*ret |= twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, 0xfe,
					TWL4030_KEYPAD_KEYP_IMR2);*/
		/* enable missed event tracking */
		/*ret |= twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, 0x20,
					TWL4030_KEYPAD_KEYP_SMS);*/
		/* enable interrupt generation on rising and falling */
		/* this is a workaround for qemu twl4030 emulation */
		ret |= twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, 0x57,
					TWL4030_KEYPAD_KEYP_EDR);
		/* enable ISR clear on read */
		ret |= twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, 0x05,
					TWL4030_KEYPAD_KEYP_SIH_CTRL);
	}
	return ret;
}

void rx51_kp_fill(u8 k, u8 mods)
{
	if (mods & 2) { /* fn */
		k = keymap[k+64];
	} else {
		k = keymap[k];
		if (mods & 4) { /* shift */
			if (k >= 'a' && k <= 'z')
				k += 'A' - 'a';
			else if (k == '.')
				k = ':';
			else if (k == ',')
				k = ';';
		}
	}
	keybuf[keybuf_tail++] = k;
	keybuf_tail %= KEYBUF_SIZE;
}

int rx51_kp_tstc(void)
{
	u8 c, r, dk, i;
	u8 intr;
	u8 mods;

	/* twl4030 remembers up to 2 events */
	for (i = 0; i < 2; i++)	{

		/* check interrupt register for events */
		twl4030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &intr,
				TWL4030_KEYPAD_KEYP_ISR1+(2*i));

		if (intr&1) { /* got an event */

			/* read the key state */
			i2c_read(TWL4030_CHIP_KEYPAD,
				TWL4030_KEYPAD_FULL_CODE_7_0, 1, keys, 8);

			/* cut out modifier keys from the keystate */
			mods = keys[4] >> 4;
			keys[4] &= 0x0f;

			for (c = 0; c < 8; c++) {

				/* get newly pressed keys only */
				dk = ((keys[c] ^ old_keys[c])&keys[c]);
				old_keys[c] = keys[c];

				/* fill the keybuf */
				for (r = 0; r < 8; r++) {
					if (dk&1)
						rx51_kp_fill((c*8)+r, mods);
					dk = dk >> 1;
				}
			}
		}
	}
	return (KEYBUF_SIZE + keybuf_tail - keybuf_head)%KEYBUF_SIZE;
}

int rx51_kp_getc(void)
{
	keybuf_head %= KEYBUF_SIZE;
	while (!rx51_kp_tstc())
		;
	return keybuf[keybuf_head++];
}
