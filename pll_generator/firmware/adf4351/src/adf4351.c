#include "adf4351.h"

spi_inst_t * adf4351_spi;

void adf_write_reg(uint32_t reg) {
    if (adf4351_spi == NULL) {
        return; 
    }
    // Data is clocked MSB first, 32 bits; LE rising edge latches into register
    uint8_t buf[4];
    buf[0] = (reg >> 24) & 0xFF;
    buf[1] = (reg >> 16) & 0xFF;
    buf[2] = (reg >> 8) & 0xFF;
    buf[3] = (reg) & 0xFF;

    // Ensure LE low
    gpio_put(PIN_PLL_LE, 0);
    sleep_us(1);

    spi_write_blocking(adf4351_spi, buf, 4);

    // Pulse LE high to latch
    gpio_put(PIN_PLL_LE, 1);
    sleep_us(1);
    gpio_put(PIN_PLL_LE, 0);
    sleep_us(1);
}

bool adf_init(spi_inst_t *spi) {
    if (spi == NULL) {
        return false;
    }
    adf4351_spi = spi;

    // init pins
    gpio_init(PIN_PLL_LE);
    gpio_set_dir(PIN_PLL_LE, GPIO_OUT);
    gpio_put(PIN_PLL_LE, 0);

    gpio_init(PIN_PLL_CE);
    gpio_set_dir(PIN_PLL_CE, GPIO_OUT);
    gpio_put(PIN_PLL_CE, 0);

    gpio_init(PIN_PLL_LD);
    gpio_set_dir(PIN_PLL_LD, GPIO_IN);

    // Write R5..R0 as initialization (datasheet requires writing R5 first)
    // R5..R2 use library defaults; R1 and R0 will be set by adf_set_frequency
    // adf_write_reg(REG_R5); // reg 5 (control bits 101)
    // adf_write_reg(REG_R4); // reg 4 (control bits 100)
    // adf_write_reg(REG_R3); // reg 3 (control bits 011)
    // adf_write_reg(REG_R2); // reg 2 (control bits 010)
    // sleep_ms(10);
    return true;
}

bool adf_is_locked(void) {
    return gpio_get(PIN_PLL_LD);
}

void adf_set_frequency(double freq_hz) {
    if (freq_hz < 35e6 || freq_hz > 4400e6) {
        printf("Frecuencia fuera de rango\n");
        return;
    }
    // Compute appropriate RF output divider so that VCO is within 2200..4400 MHz
    const double VCO_MIN = 2200e6;
    const double VCO_MAX = 4400e6;
    const int dividers[] = {1,2,4,8,16,32,64};
    int rf_div = 1;
    double vco;
    for (int i = 0; i < (int)(sizeof(dividers)/sizeof(dividers[0])); i++) {
        double candidate = freq_hz * dividers[i];
        if (candidate >= VCO_MIN && candidate <= VCO_MAX) {
            rf_div = dividers[i];
            vco = candidate;
            break;
        }
    }

    // PFD frequency: REFIN * (1 + doublerBit) / Rcounter / (1 + div2bit)
    // For simplicity we'll assume no doubler, no /2, Rcounter=1
    double ref = REFIN_HZ;
    double rcounter = 1.0;
    double fPFD = ref / rcounter;

    // Choose MOD (12 bit max = 4095). Higher MOD gives better resolution; choose 4095.
    uint32_t mod_value = 4095;

    // Calculate N = vco / fPFD
    double N = vco / fPFD;
    uint32_t int_part = (uint32_t)floor(N);
    double frac = N - (double)int_part;
    uint32_t frac_part = (uint32_t)round(frac * mod_value);
    if (frac_part >= mod_value) {
        frac_part = 0;
        int_part += 1;
    }

    // prescaler: use 4/5 (PR1=0) if VCO <= 3600 MHz, else 8/9 (PR1=1)
    uint32_t prescaler_bit = (vco > 3600e6) ? 1U : 0U;

    // Build R0 and R1 per datasheet bit positions
    // R0: [INT(16 bits) << 15] | [FRAC(12 bits) << 3] | reg = 0
    uint32_t r0 = ((int_part & 0xFFFF) << 15) | ((frac_part & 0x0FFF) << 3) | 0x0; // control bits 000

    // R1: [phase (12 bits) << 15] | [MOD(12 bits) << 3] | reg=1
    uint32_t phase = 0; // default
    uint32_t r1 = ((phase & 0x0FFF) << 15) | ((mod_value & 0x0FFF) << 3) | 0x1;

    // Set prescaler bit (DB27) in R1 if needed
    if (prescaler_bit) r1 |= (1U << 27);

    // Some modules require additional bits in R4 for the RF output divider setting.
    // R4 bits for RF output divider: bits [22:20] often control RF output divider (example mapping)
    // We'll modify REG_R4's low portion to set rf divider exponent (0..6)
    uint32_t r4 = REG_R4;
    // Clear bits 22:20
    r4 &= ~(0x7 << 20);
    // map rf_div to code: 1->0,2->1,4->2,8->3,16->4,32->5,64->6
    int code = 0;
    switch (rf_div) {
        case 1: code = 0; break;
        case 2: code = 1; break;
        case 4: code = 2; break;
        case 8: code = 3; break;
        case 16: code = 4; break;
        case 32: code = 5; break;
        case 64: code = 6; break;
    }
    r4 |= ((uint32_t)code & 0x7) << 20;
    // ensure control bits for R4 (100)
    r4 = (r4 & ~0x7) | 0x4;

    // Build other registers with proper control bits in LSBs
    uint32_t r2 = (REG_R2 & ~0x7) | 0x2;
    uint32_t r3 = (REG_R3 & ~0x7) | 0x3;
    uint32_t r5 = (REG_R5 & ~0x7) | 0x5;

    // Now write R5..R0 (datasheet requires R5 first)
    adf_write_reg(r5);
    adf_write_reg(r4);
    adf_write_reg(r3);
    adf_write_reg(r2);
    adf_write_reg(r1);
    adf_write_reg(r1);

    // Increase SPI speed after init
    // spi_set_baudrate(adf4351_spi, 8000000);
}