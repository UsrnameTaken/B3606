/* Copyright (C) 2015 Baruch Even
 *
 * This file is part of the B3603 alternative firmware.
 *
 *  B3603 alternative firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  B3603 alternative firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with B3603 alternative firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#define FW_VERSION "0.1"

#include "stm8s.h"
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "display.h"
#include "fixedpoint.h"
#include "uart.h"
#include "eeprom.h"
#include "outputs.h"
#include "calibrate.h"
#include "config.h"
#include "parse.h"

#define CAP_VMIN FLOAT_TO_FIXED(0.01) // 10mV
#define CAP_VMAX FLOAT_TO_FIXED(35.0) // 35 V
#define CAP_VSTEP FLOAT_TO_FIXED(0.01) // 10mV

#define CAP_CMIN FLOAT_TO_FIXED(0.001) // 1 mA
#define CAP_CMAX FLOAT_TO_FIXED(3) // 3 A
#define CAP_CSTEP FLOAT_TO_FIXED(0.001) // 1 mA

cfg_system_t cfg_system;
cfg_output_t cfg_output;
state_t state;

inline iwatchdog_init(void)
{
	IWDG_KR = 0xCC; // Enable IWDG
	// The default values give us about 15msec between pings
}

inline iwatchdog_tick(void)
{
	IWDG_KR = 0xAA; // Reset the counter
}

void commit_output()
{
	output_commit(&cfg_output, &cfg_system, state.constant_current);
}

void set_name(uint8_t *name)
{
	uint8_t idx;

	for (idx = 0; name[idx] != 0; idx++) {
		if (!isprint(name[idx]))
			name[idx] = '.'; // Eliminate non-printable chars
	}

	strncpy(cfg_system.name, name, sizeof(cfg_system.name));
	cfg_system.name[sizeof(cfg_system.name)-1] = 0;

	uart_write_str("SNAME: ");
	uart_write_str(cfg_system.name);
	uart_write_str("\r\n");
}

void set_output(uint8_t *s)
{
	if (s[1] != 0) {
		uart_write_str("OUTPUT takes either 0 for OFF or 1 for ON, received: \"");
		uart_write_str(s);
		uart_write_str("\"\r\n");
		return;
	}

	if (s[0] == '0') {
		cfg_system.output = 0;
		uart_write_str("OUTPUT: OFF\r\n");
	} else if (s[0] == '1') {
		cfg_system.output = 1;
		uart_write_str("OUTPUT: ON\r\n");
	} else {
		uart_write_str("OUTPUT takes either 0 for OFF or 1 for ON, received: \"");
		uart_write_str(s);
		uart_write_str("\"\r\n");
	}

	if (cfg_system.autocommit) {
		commit_output();
	} else {
		uart_write_str("AUTOCOMMIT OFF: CHANGE PENDING ON COMMIT\r\n");
	}
}

void set_voltage(uint8_t *s)
{
	fixed_t val;

	val = parse_fixed_point(s);
	if (val == 0xFFFF)
		return;

	if (val > CAP_VMAX) {
		uart_write_str("VOLTAGE VALUE TOO HIGH\r\n");
		return;
	}
	if (val < CAP_VMIN) {
		uart_write_str("VOLTAGE VALUE TOO LOW\r\n");
		return;
	}

	uart_write_str("VOLTAGE: SET ");
	uart_write_fixed_point(val);
	uart_write_str("\r\n");
	cfg_output.vset = val;

	if (cfg_system.autocommit) {
		commit_output();
	} else {
		uart_write_str("AUTOCOMMIT OFF: CHANGE PENDING ON COMMIT\r\n");
	}
}

void set_current(uint8_t *s)
{
	fixed_t val;

	val = parse_fixed_point(s);
	if (val == 0xFFFF)
		return;

	if (val > CAP_CMAX) {
		uart_write_str("CURRENT VALUE TOO HIGH\r\n");
		return;
	}
	if (val < CAP_CMIN) {
		uart_write_str("CURRENT VALUE TOO LOW\r\n");
		return;
	}

	uart_write_str("CURRENT: SET ");
	uart_write_fixed_point(val);
	uart_write_str("\r\n");
	cfg_output.cset = val;

	if (cfg_system.autocommit) {
		commit_output();
	} else {
		uart_write_str("AUTOCOMMIT OFF: CHANGE PENDING ON COMMIT\r\n");
	}
}

void set_autocommit(uint8_t *s)
{
	if (strcmp(s, "1") == 0 || strcmp(s, "YES") == 0) {
		cfg_system.autocommit = 1;
		uart_write_str("AUTOCOMMIT: YES\r\n");
	} else if (strcmp(s, "0") == 0 || strcmp(s, "NO") == 0) {
		cfg_system.autocommit = 0;
		uart_write_str("AUTOCOMMIT: NO\r\n");
	} else {
		uart_write_str("UNKNOWN AUTOCOMMIT ARG: ");
		uart_write_str(s);
		uart_write_str("\r\n");
	}
}

void process_input()
{
	// Eliminate the CR/LF character
	uart_read_buf[uart_read_len-1] = 0;

	if (strcmp(uart_read_buf, "MODEL") == 0) {
		uart_write_str("MODEL: B3606\r\n");
	} else if (strcmp(uart_read_buf, "VERSION") == 0) {
		uart_write_str("VERSION: " FW_VERSION "\r\n");
	} else if (strcmp(uart_read_buf, "SYSTEM") == 0) {
		uart_write_str("MODEL: B3606\r\n");
		uart_write_str("VERSION: " FW_VERSION "\r\n");
		uart_write_str("NAME: ");
		uart_write_str(cfg_system.name);
		uart_write_str("\r\n");
		uart_write_str("ONSTARTUP: ");
		uart_write_str(cfg_system.default_on ? "ON" : "OFF");
		uart_write_str("\r\n");
		uart_write_str("AUTOCOMMIT: ");
		uart_write_str(cfg_system.autocommit ? "YES" : "NO");
		uart_write_str("\r\n");
	} else if (strcmp(uart_read_buf, "CALIBRATION") == 0) {
		uart_write_str("CALIBRATE VIN ADC: ");
		uart_write_fixed_point(cfg_system.vin_adc.a);
		uart_write_ch('/');
		uart_write_fixed_point(cfg_system.vin_adc.b);
		uart_write_str("\r\n");
		uart_write_str("CALIBRATE VOUT ADC: ");
		uart_write_fixed_point(cfg_system.vout_adc.a);
		uart_write_ch('/');
		uart_write_fixed_point(cfg_system.vout_adc.b);
		uart_write_str("\r\n");
		uart_write_str("CALIBRATE COUT ADC: ");
		uart_write_fixed_point(cfg_system.cout_adc.a);
		uart_write_ch('/');
		uart_write_fixed_point(cfg_system.cout_adc.b);
		uart_write_str("\r\n");
		uart_write_str("CALIBRATE VOUT PWM: ");
		uart_write_fixed_point(cfg_system.vout_pwm.a);
		uart_write_ch('/');
		uart_write_fixed_point(cfg_system.vout_pwm.b);
		uart_write_str("\r\n");
		uart_write_str("CALIBRATE COUT PWM: ");
		uart_write_fixed_point(cfg_system.cout_pwm.a);
		uart_write_ch('/');
		uart_write_fixed_point(cfg_system.cout_pwm.b);
		uart_write_str("\r\n");
	} else if (strcmp(uart_read_buf, "VLIST") == 0) {
		uart_write_str("VLIST:\r\nVOLTAGE MIN: ");
		uart_write_fixed_point(CAP_VMIN);
		uart_write_str("\r\nVOLTAGE MAX: ");
		uart_write_fixed_point(CAP_VMAX);
		uart_write_str("\r\nVOLTAGE STEP: ");
		uart_write_fixed_point(CAP_VSTEP);
		uart_write_str("\r\n");
	} else if (strcmp(uart_read_buf, "CLIST") == 0) {
		uart_write_str("CLIST:\r\nCURRENT MIN: ");
		uart_write_fixed_point(CAP_CMIN);
		uart_write_str("\r\nCURRENT MAX: ");
		uart_write_fixed_point(CAP_CMAX);
		uart_write_str("\r\nCURRENT STEP: ");
		uart_write_fixed_point(CAP_CSTEP);
		uart_write_str("\r\n");
	} else if (strcmp(uart_read_buf, "CONFIG") == 0) {
		uart_write_str("CONFIG:\r\nOUTPUT: ");
		uart_write_str(cfg_system.output ? "ON" : "OFF");
		uart_write_str("\r\nVOLTAGE SET: ");
		uart_write_fixed_point(cfg_output.vset);
		uart_write_str("\r\nCURRENT SET: ");
		uart_write_fixed_point(cfg_output.cset);
		uart_write_str("\r\nVOLTAGE SHUTDOWN: ");
		if (cfg_output.vshutdown == 0)
			uart_write_str("DISABLED");
		else
			uart_write_fixed_point(cfg_output.vshutdown);
		uart_write_str("\r\nCURRENT SHUTDOWN: ");
		if (cfg_output.cshutdown == 0)
			uart_write_str("DISABLED");
		else
			uart_write_fixed_point(cfg_output.cshutdown);
		uart_write_str("\r\n");
	} else if (strcmp(uart_read_buf, "STATUS") == 0) {
		uart_write_str("STATUS:\r\nOUTPUT: ");
		uart_write_str(cfg_system.output ? "ON" : "OFF");
		uart_write_str("\r\nVOLTAGE IN: ");
		uart_write_fixed_point(state.vin);
		uart_write_str("\r\nVOLTAGE OUT: ");
		uart_write_fixed_point(cfg_system.output ? state.vout : 0);
		uart_write_str("\r\nCURRENT OUT: ");
		uart_write_fixed_point(cfg_system.output ? state.cout : 0);
		uart_write_str("\r\nCONSTANT: ");
		uart_write_str(state.constant_current ? "CURRENT" : "VOLTAGE");
		uart_write_str("\r\n");
	} else if (strcmp(uart_read_buf, "RSTATUS") == 0) {
		uart_write_str("RSTATUS:\r\nOUTPUT: ");
		uart_write_str(cfg_system.output ? "ON" : "OFF");
		uart_write_str("\r\nVOLTAGE IN ADC: ");
		uart_write_int(state.vin_raw);
		uart_write_str("\r\nVOLTAGE OUT ADC: ");
		uart_write_int(state.vout_raw);
		uart_write_str("\r\nCURRENT OUT ADC: ");
		uart_write_int(state.cout_raw);
		uart_write_str("\r\nCONSTANT: ");
		uart_write_str(state.constant_current ? "CURRENT" : "VOLTAGE");
		uart_write_str("\r\n");
	} else if (strcmp(uart_read_buf, "COMMIT") == 0) {
		commit_output();
	} else if (strcmp(uart_read_buf, "SAVE") == 0) {
		config_save_system(&cfg_system);
		config_save_output(&cfg_output);
		uart_write_str("SAVED\r\n");
#if DEBUG
	} else if (strcmp(uart_read_buf, "STUCK") == 0) {
		// Allows debugging of the IWDG feature
		uart_write_str("STUCK\r\n");
		while (1)
			uart_write_from_buf(); // Flush write buf outside
#endif
	} else {
		// Process commands with arguments
		uint8_t idx;
		uint8_t space_found = 0;

		for (idx = 0; idx < uart_read_len; idx++) {
			if (uart_read_buf[idx] == ' ') {
				uart_read_buf[idx] = 0;
				space_found = 1;
				break;
			}
		}

		if (space_found) {
			if (strcmp(uart_read_buf, "SNAME") == 0) {
				set_name(uart_read_buf + idx + 1);
			} else if (strcmp(uart_read_buf, "OUTPUT") == 0) {
				set_output(uart_read_buf + idx + 1);
			} else if (strcmp(uart_read_buf, "VOLTAGE") == 0) {
				set_voltage(uart_read_buf + idx + 1);
			} else if (strcmp(uart_read_buf, "CURRENT") == 0) {
				set_current(uart_read_buf + idx + 1);
			} else if (strcmp(uart_read_buf, "AUTOCOMMIT") == 0) {
				set_autocommit(uart_read_buf + idx + 1);
			} else if (strcmp(uart_read_buf, "CALVIN1") == 0) {
				calibrate_vin(1, parse_fixed_point(uart_read_buf+idx+1), state.vin_raw, &cfg_system.vin_adc);
			} else if (strcmp(uart_read_buf, "CALVIN2") == 0) {
				calibrate_vin(2, parse_fixed_point(uart_read_buf+idx+1), state.vin_raw, &cfg_system.vin_adc);
			} else if (strcmp(uart_read_buf, "CALVOUT1") == 0) {
				calibrate_vout(1, parse_fixed_point(uart_read_buf+idx+1), state.vout_raw, &cfg_system.vout_adc);
			} else if (strcmp(uart_read_buf, "CALVOUT2") == 0) {
				calibrate_vout(2, parse_fixed_point(uart_read_buf+idx+1), state.vout_raw, &cfg_system.vout_adc);
			} else {
				uart_write_str("UNKNOWN COMMAND!\r\n");
			}
		} else {
			uart_write_str("UNKNOWN COMMAND\r\n");
		}
	}

	uart_read_len = 0;
	read_newline = 0;
}

inline void clk_init()
{
	CLK_CKDIVR = 0x00; // Set the frequency to 16 MHz
}

inline void pinout_init()
{
	// PA1 is 74HC595 SHCP, output
	// PA2 is 74HC595 STCP, output
	// PA3 is CV/CC leds, output (& input to disable)
	PA_ODR = 0;
	PA_DDR = (1<<1) | (1<<2);
	PA_CR1 = (1<<1) | (1<<2) | (1<<3);
	PA_CR2 = (1<<1) | (1<<2) | (1<<3);

	// PB4 is Enable control, output
	// PB5 is CV/CC sense, input
	PB_ODR = (1<<4); // For safety we start with off-state
	PB_DDR = (1<<4);
	PB_CR1 = (1<<4);
	PB_CR2 = 0;

	// PC3 is unknown, input
	// PC4 is Iout sense, input adc, AIN2
	// PC5 is Vout control, output
	// PC6 is Iout control, output
	// PC7 is Button 1, input
	PC_ODR = 0;
	PC_DDR = (1<<5) || (1<<6);
	PC_CR1 = (1<<7); // For the button
	PC_CR2 = (1<<5) | (1<<6);

	// PD1 is Button 2, input
	// PD2 is Vout sense, input adc, AIN3
	// PD3 is Vin sense, input adc, AIN4
	// PD4 is 74HC595 DS, output
	PD_DDR = (1<<4);
	PD_CR1 = (1<<1) | (1<<4); // For the button
	PD_CR2 = (1<<4);
}

void adc_init(void)
{
	ADC1_CR1 = 0x70; // Power down, clock/18
	ADC1_CR2 = 0x08; // Right alignment
	ADC1_CR3 = 0x00;
	ADC1_CSR = 0x00;

	ADC1_TDRL = 0x0F;

	ADC1_CR1 |= 0x01; // Turn on the ADC
}

void adc_start(uint8_t channel)
{
	uint8_t csr = ADC1_CSR;
	csr &= 0x70; // Turn off EOC, Clear Channel
	csr |= channel; // Select channel
	ADC1_CSR = csr;

	ADC1_CR1 |= 1; // Trigger conversion
}

inline uint8_t adc_ready()
{
	return ADC1_CSR & 0x80;
}

void config_load(void)
{
	config_load_system(&cfg_system);
	config_load_output(&cfg_output);

	if (cfg_system.default_on)
		cfg_system.output = 1;
	else
		cfg_system.output = 0;

	state.pc3 = 1;
}

fixed_t adc_to_volt(uint16_t adc, calibrate_t *cal)
{
	fixed_t tmp;

	tmp = fixed_mult(adc, cal->a);

	if (tmp > cal->b)
		tmp -= cal->b;

	return tmp;
}

void read_state(void)
{
	uint8_t tmp;

	tmp = (PC_IDR & (1<<3)) ? 1 : 0;
	if (state.pc3 != tmp) {
		uart_write_str("PC3 is now ");
		uart_write_ch('0' + tmp);
		uart_write_str("\r\n");
		state.pc3 = tmp;
	}

	tmp = (PB_IDR & (1<<5)) ? 1 : 0;
	if (state.constant_current != tmp) {
		state.constant_current = tmp;
		output_check_state(&cfg_system, state.constant_current);
	}

	if ((ADC1_CSR & 0x0F) == 0) {
		adc_start(2);
	} else if (adc_ready()) {
		uint16_t val = ADC1_DRL;
		uint16_t valh = ADC1_DRH;
		uint8_t ch = ADC1_CSR & 0x0F;

		val |= valh << 8;

		switch (ch) {
			case 2:
				state.cout_raw = val;
				// Calculation: val * cal_cout_a * 3.3 / 1024 - cal_cout_b
				state.cout = adc_to_volt(val, &cfg_system.cout_adc);
				ch = 3;
				break;
			case 3:
				state.vout_raw = val;
				// Calculation: val * cal_vout_a * 3.3 / 1024 - cal_vout_b
				state.vout = adc_to_volt(val, &cfg_system.vout_adc);
				ch = 4;
				break;
			case 4:
				state.vin_raw = val;
				// Calculation: val * cal_vin * 3.3 / 1024
				state.vin = adc_to_volt(val, &cfg_system.vin_adc);
				ch = 2;
				{
					uint8_t ch1;
					uint8_t ch2;
					uint8_t ch3;
					uint8_t ch4;

					val = state.vin >> FIXED_SHIFT;

					ch2 = '0' + (val % 10);
					ch1 = '0' + (val / 10) % 10;

					val = state.vin & FIXED_FRACTION_MASK;
					val = fixed_mult(val, 100);

					ch4 = '0' + (val % 10);
					ch3 = '0' + (val / 10) % 10;

					display_show(ch1, 0, ch2, 1, ch3, 0, ch4, 0);
				}
				break;
		}

		adc_start(ch);
	}
}

void ensure_afr0_set(void)
{
	if ((OPT2 & 1) == 0) {
		uart_flush_writes();
		if (eeprom_set_afr0()) {
			uart_write_str("AFR0 set, reseting the unit\r\n");
			uart_flush_writes();
			iwatchdog_init();
			while (1); // Force a reset in a few msec
		}
		else {
			uart_write_str("AFR0 not set and programming failed!\r\n");
		}
	}
}

int main()
{
	unsigned long i = 0;

	pinout_init();
	clk_init();
	uart_init();
	pwm_init();
	adc_init();

	config_load();

	uart_write_str("\r\nB3606 starting: Version " FW_VERSION "\r\n");

	ensure_afr0_set();

	iwatchdog_init();
	commit_output();

	do {
		iwatchdog_tick();
		uart_write_from_buf();

		read_state();
		display_refresh();

		uart_read_to_buf();
		if (read_newline) {
			process_input();
		}
	} while(1);
}
