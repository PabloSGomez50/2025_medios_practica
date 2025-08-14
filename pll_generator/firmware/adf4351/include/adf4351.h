#ifndef ADF4351_H
#define ADF4351_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

#define PIN_PLL_LE   9
#define PIN_PLL_CE   8
#define PIN_PLL_LD   7

#define REFIN_HZ 25000000.0

#define REG_R5 0x00580005 // Register 5: Power-down control, etc. (control bits = 101)
#define REG_R4 0x0800801E // Register 4: Set to default band select and ref settings (control bits = 100)
#define REG_R3 0x000004B3 // Register 3: Reference doubler/div2/phase etc (control bits = 011)
#define REG_R2 0x00004E42 // Register 2: Feedback path, muxout default (control bits = 010)

bool adf_init(spi_inst_t *spi);
void adf_write_reg(uint32_t reg);
void adf_set_frequency(double freq_hz);
bool adf_is_locked(void);

#endif