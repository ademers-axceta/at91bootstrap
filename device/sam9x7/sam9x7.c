// Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries
//
// SPDX-License-Identifier: MIT

#include "common.h"
#include "hardware.h"
#include "arch/at91_sfr.h"
#include "arch/at91_rstc.h"
#include "arch/at91_pmc/pmc.h"
#include "arch/at91_smc.h"
#include "arch/at91_pio.h"
#include "arch/at91_ddrsdrc.h"
#include "gpio.h"
#include "pmc.h"
#include "usart.h"
#include "debug.h"
#include "ddramc.h"
#include "timer.h"
#include "watchdog.h"
#include "string.h"
#include "sam9x7_board.h"
#include "board_hw_info.h"
#include "twi.h"
#include "flexcom.h"
#include "board.h"
#include "led.h"

__attribute__((weak)) void wilc_pwrseq(void);
__attribute__((weak)) void at91_can_stdby_dis(void);

#define PLLA_DIV 0
#define PLLA_COUNT 0x3f
#define PLLA_FRACR(_p, _q) \
	((unsigned int)((((unsigned long)(_p)) << 22) / (_q)))

#define FLEXCOM_USART_INDEX (CONFIG_CONSOLE_INDEX - 1)

#if defined(CONFIG_TWI) || CONFIG_CONSOLE_INDEX != 0
static struct at91_flexcom flexcoms[] = {
	{AT91C_ID_FLEXCOM0, FLEXCOM_TWI, AT91C_BASE_FLEXCOM0},
	{AT91C_ID_FLEXCOM1, FLEXCOM_TWI, AT91C_BASE_FLEXCOM1},
	{AT91C_ID_FLEXCOM2, FLEXCOM_TWI, AT91C_BASE_FLEXCOM2},
	{AT91C_ID_FLEXCOM3, FLEXCOM_TWI, AT91C_BASE_FLEXCOM3},
	{AT91C_ID_FLEXCOM4, FLEXCOM_TWI, AT91C_BASE_FLEXCOM4},
	{AT91C_ID_FLEXCOM5, FLEXCOM_TWI, AT91C_BASE_FLEXCOM5},
	{AT91C_ID_FLEXCOM6, FLEXCOM_TWI, AT91C_BASE_FLEXCOM6},
	{AT91C_ID_FLEXCOM7, FLEXCOM_TWI, AT91C_BASE_FLEXCOM7},
	{AT91C_ID_FLEXCOM8, FLEXCOM_TWI, AT91C_BASE_FLEXCOM8},
	{AT91C_ID_FLEXCOM9, FLEXCOM_TWI, AT91C_BASE_FLEXCOM9},
	{AT91C_ID_FLEXCOM10, FLEXCOM_TWI, AT91C_BASE_FLEXCOM10},
	{AT91C_ID_FLEXCOM11, FLEXCOM_TWI, AT91C_BASE_FLEXCOM11},
	{AT91C_ID_FLEXCOM12, FLEXCOM_TWI, AT91C_BASE_FLEXCOM12},
};
#endif

unsigned int usart_base = AT91C_BASE_DBGU;

static void at91_dbgu_hw_init(void)
{
	const struct pio_desc dbgu_pins[3] = {
#if CONFIG_CONSOLE_INDEX == 0
		/* DBGU */
		{"RXD", AT91C_PIN_PA(26), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"TXD", AT91C_PIN_PA(27), 0, PIO_DEFAULT, PIO_PERIPH_A},
#elif CONFIG_CONSOLE_INDEX == 1
		/* FLEXCOM0 */
		{"FLEXCOM0_RXD", AT91C_PIN_PA(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLEXCOM0_TXD", AT91C_PIN_PA(30), 0, PIO_DEFAULT, PIO_PERIPH_A},
#elif CONFIG_CONSOLE_INDEX == 2
		/* FLEXCOM1 */
		{"FLEXCOM1_RXD", AT91C_PIN_PA(29), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLEXCOM1_TXD", AT91C_PIN_PA(28), 0, PIO_DEFAULT, PIO_PERIPH_A},
#elif CONFIG_CONSOLE_INDEX == 3
		/* FLEXCOM2 */
		{"FLEXCOM2_RXD", AT91C_PIN_PA(14), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLEXCOM2_TXD", AT91C_PIN_PA(13), 0, PIO_DEFAULT, PIO_PERIPH_A},
#elif CONFIG_CONSOLE_INDEX == 4
		/* FLEXCOM3 */
		{"FLEXCOM3_RXD", AT91C_PIN_PC(23), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"FLEXCOM3_TXD", AT91C_PIN_PC(22), 0, PIO_DEFAULT, PIO_PERIPH_B},
#elif CONFIG_CONSOLE_INDEX == 5
		/* FLEXCOM4 */
		{"FLEXCOM4_RXD", AT91C_PIN_PA(9), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLEXCOM4_TXD", AT91C_PIN_PA(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
#elif CONFIG_CONSOLE_INDEX == 6
		/* FLEXCOM5 */
		{"FLEXCOM5_RXD", AT91C_PIN_PA(15), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"FLEXCOM5_TXD", AT91C_PIN_PA(16), 0, PIO_DEFAULT, PIO_PERIPH_B},
#elif CONFIG_CONSOLE_INDEX == 7
		/* FLEXCOM6 */
		{"FLEXCOM6_RXD", AT91C_PIN_PA(25), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLEXCOM6_TXD", AT91C_PIN_PA(24), 0, PIO_DEFAULT, PIO_PERIPH_A},
#elif CONFIG_CONSOLE_INDEX == 8
		/* FLEXCOM7 */
		{"FLEXCOM7_RXD", AT91C_PIN_PC(1), 0, PIO_DEFAULT, PIO_PERIPH_C},
		{"FLEXCOM7_TXD", AT91C_PIN_PC(0), 0, PIO_DEFAULT, PIO_PERIPH_C},
#elif CONFIG_CONSOLE_INDEX == 9
		/* FLEXCOM8 */
		{"FLEXCOM8_RXD", AT91C_PIN_PB(5), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"FLEXCOM8_TXD", AT91C_PIN_PB(4), 0, PIO_DEFAULT, PIO_PERIPH_B},
#elif CONFIG_CONSOLE_INDEX == 10
		/* FLEXCOM9 */
		{"FLEXCOM9_RXD", AT91C_PIN_PC(9), 0, PIO_DEFAULT, PIO_PERIPH_C},
		{"FLEXCOM9_TXD", AT91C_PIN_PC(8), 0, PIO_DEFAULT, PIO_PERIPH_C},
#elif CONFIG_CONSOLE_INDEX == 11
		/* FLEXCOM10 */
		{"FLEXCOM10_RXD", AT91C_PIN_PC(17), 0, PIO_DEFAULT, PIO_PERIPH_C},
		{"FLEXCOM10_TXD", AT91C_PIN_PC(16), 0, PIO_DEFAULT, PIO_PERIPH_C},
#elif CONFIG_CONSOLE_INDEX == 12
		/* FLEXCOM11 */
		{"FLEXCOM11_RXD", AT91C_PIN_PB(16), 0, PIO_DEFAULT, PIO_PERIPH_C},
		{"FLEXCOM11_TXD", AT91C_PIN_PB(15), 0, PIO_DEFAULT, PIO_PERIPH_C},
#elif CONFIG_CONSOLE_INDEX == 13
		/* FLEXCOM12 */
		{"FLEXCOM12_RXD", AT91C_PIN_PB(18), 0, PIO_DEFAULT, PIO_PERIPH_C},
		{"FLEXCOM12_TXD", AT91C_PIN_PB(17), 0, PIO_DEFAULT, PIO_PERIPH_C},
#else
#error "Invalid CONSOLE_INDEX was chosen"
#endif
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pio_configure(dbgu_pins);
#if CONFIG_CONSOLE_INDEX == 0
	pmc_enable_periph_clock(AT91C_ID_DBGU, PMC_PERIPH_CLK_DIVIDER_NA);
#else
	usart_base = flexcoms[FLEXCOM_USART_INDEX].addr;
	flexcoms[FLEXCOM_USART_INDEX].mode = FLEXCOM_USART;
	flexcoms[FLEXCOM_USART_INDEX].addr = usart_base - AT91C_OFFSET_FLEXCOM_USART;
	flexcom_init(FLEXCOM_USART_INDEX);
	pmc_enable_periph_clock(flexcoms[FLEXCOM_USART_INDEX].id, PMC_PERIPH_CLK_DIVIDER_NA);
#endif
}

static void initialize_dbgu(void)
{
	at91_dbgu_hw_init();
	usart_init(BAUDRATE(MASTER_CLOCK, BAUD_RATE));
}

#if defined CONFIG_TWI
#if defined(CONFIG_FLEXCOM0) || defined(CONFIG_FLEXCOM1) || defined(CONFIG_FLEXCOM2) || \
	defined(CONFIG_FLEXCOM3) || defined(CONFIG_FLEXCOM4) || defined(CONFIG_FLEXCOM5) || \
	defined(CONFIG_FLEXCOM6) || defined(CONFIG_FLEXCOM7) || defined(CONFIG_FLEXCOM8) || \
	defined(CONFIG_FLEXCOM9) || defined(CONFIG_FLEXCOM10) || defined(CONFIG_FLEXCOM11) || \
	defined(CONFIG_FLEXCOM12)
static unsigned int at91_flexcom_twi_hw_init(unsigned int index)
{
	const struct pio_desc flx_pins[][3] = {
		{ // FLEXCOM0
			{"FLX_IO0", AT91C_PIN_PA(30), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLX_IO1", AT91C_PIN_PA(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM1
			{"FLX_IO0", AT91C_PIN_PA(28), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLX_IO1", AT91C_PIN_PA(29), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM2
			{"FLX_IO0", AT91C_PIN_PA(13), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLX_IO1", AT91C_PIN_PA(14), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM3
			{"FLX_IO0", AT91C_PIN_PC(22), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{"FLX_IO1", AT91C_PIN_PC(23), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM4
			{"FLX_IO0", AT91C_PIN_PA(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLX_IO1", AT91C_PIN_PA(9), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM5
			{"FLX_IO0", AT91C_PIN_PA(16), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{"FLX_IO1", AT91C_PIN_PA(15), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM6
			{"FLX_IO0", AT91C_PIN_PA(24), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{"FLX_IO1", AT91C_PIN_PA(25), 0, PIO_DEFAULT, PIO_PERIPH_A},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM7
			{"FLX_IO0", AT91C_PIN_PC(0), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLX_IO1", AT91C_PIN_PC(1), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM8
			{"FLX_IO0", AT91C_PIN_PB(4), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{"FLX_IO1", AT91C_PIN_PB(5), 0, PIO_DEFAULT, PIO_PERIPH_B},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM9
			{"FLX_IO0", AT91C_PIN_PC(8), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLX_IO1", AT91C_PIN_PC(9), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM10
			{"FLX_IO0", AT91C_PIN_PC(16), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLX_IO1", AT91C_PIN_PC(17), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM11
			{"FLX_IO0", AT91C_PIN_PB(15), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLX_IO1", AT91C_PIN_PB(16), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
		{ // FLEXCOM12
			{"FLX_IO0", AT91C_PIN_PB(17), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{"FLX_IO1", AT91C_PIN_PB(18), 0, PIO_DEFAULT, PIO_PERIPH_C},
			{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
		},
	};

	if (index + 1 == CONFIG_CONSOLE_INDEX) {
		dbg_very_loud("\nFLEXCOM %d is used in UART mode, \
			      config to TWI mode ignored!\n\n", index);
		return 0;
	}

	pio_configure(flx_pins[index]);

	pmc_enable_periph_clock(flexcoms[index].id, PMC_PERIPH_CLK_DIVIDER_NA);

	flexcom_init(index);

	return flexcom_get_regmap(index);
}
#endif

void twi_init(void)
{
#ifdef CONFIG_FLEXCOM0
	twi_bus_init(at91_flexcom_twi_hw_init, 0);
#endif
#ifdef CONFIG_FLEXCOM1
	twi_bus_init(at91_flexcom_twi_hw_init, 1);
#endif
#ifdef CONFIG_FLEXCOM2
	twi_bus_init(at91_flexcom_twi_hw_init, 2);
#endif
#ifdef CONFIG_FLEXCOM3
	twi_bus_init(at91_flexcom_twi_hw_init, 3);
#endif
#ifdef CONFIG_FLEXCOM4
	twi_bus_init(at91_flexcom_twi_hw_init, 4);
#endif
#ifdef CONFIG_FLEXCOM5
	twi_bus_init(at91_flexcom_twi_hw_init, 5);
#endif
#ifdef CONFIG_FLEXCOM6
	twi_bus_init(at91_flexcom_twi_hw_init, 6);
#endif
#ifdef CONFIG_FLEXCOM7
	twi_bus_init(at91_flexcom_twi_hw_init, 7);
#endif
#ifdef CONFIG_FLEXCOM8
	twi_bus_init(at91_flexcom_twi_hw_init, 8);
#endif
#ifdef CONFIG_FLEXCOM9
	twi_bus_init(at91_flexcom_twi_hw_init, 9);
#endif
#ifdef CONFIG_FLEXCOM10
	twi_bus_init(at91_flexcom_twi_hw_init, 10);
#endif
#ifdef CONFIG_FLEXCOM11
	twi_bus_init(at91_flexcom_twi_hw_init, 11);
#endif
#ifdef CONFIG_FLEXCOM12
	twi_bus_init(at91_flexcom_twi_hw_init, 12);
#endif
}
#endif

void hw_init(void)
{
	unsigned int reg;
	struct pmc_pll_cfg plla_config;

	/* Disable watchdog */
	at91_disable_wdt();

#ifdef CONFIG_LED_ON_BOARD
	at91_leds_init();
#endif

	/* Configure & Enable PLLA */
	plla_config.mul = 65;
	plla_config.div = PLLA_DIV;
	plla_config.count = PLLA_COUNT;
	plla_config.fracr = 0x2aaaab;
	plla_config.acr = AT91C_PLL_ACR_DEFAULT_PLLA;
	pmc_sam9x60_cfg_pll(PLL_ID_PLLA, &plla_config);
	pmc_mck_cfg_set(0, BOARD_PRESCALER_PLLA,
			AT91C_PMC_PRES | AT91C_PMC_MDIV | AT91C_PMC_CSS);

	/* Enable PLLADIV2 */
	pmc_sam9x60_cfg_pll(PLL_ID_PLLADIV2, &plla_config);

#if defined(CONFIG_TWI) || CONFIG_CONSOLE_INDEX != 0
	flexcoms_init(flexcoms);
#endif

	/* Initialize dbgu */
	initialize_dbgu();

	/* Enable External Reset */
	writel(AT91C_RSTC_KEY_UNLOCK | AT91C_RSTC_URSTEN, AT91C_BASE_RSTC + RSTC_RMR);

	/* Init timer */
	timer_init();

#ifdef CONFIG_TWI
	twi_init();
#endif

#if defined(CONFIG_DDR3)
	reg = SFR_CAL1_TEST | SFR_CAL1_CALN(0x6) | SFR_CAL1_CALP(0x8);
	writel(reg, AT91C_BASE_SFR + SFR_CAL1);
#elif defined(CONFIG_DDR2)
	reg = SFR_CAL1_TEST | SFR_CAL1_CALN(0x4) | SFR_CAL1_CALP(0x9);
	writel(reg, AT91C_BASE_SFR + SFR_CAL1);
#endif

	reg = readl(AT91C_BASE_SFR + SFR_DDRCFG);

#ifdef CONFIG_DDR3
	reg |= (AT91C_EBI_CS1A | AT91C_EBI_DDR_MP_EN);
	writel(reg, (AT91C_BASE_SFR + SFR_DDRCFG));
	/* Initialize DDRAM Controller */
	ddram_init();
#endif

#ifdef CONFIG_BOARD_QUIRK_SAM9X75_EB
	/* Perform the WILC initialization sequence */
	wilc_pwrseq();
	at91_can_stdby_dis();
#endif
}

#ifdef CONFIG_SDCARD
#ifdef CONFIG_OF_LIBFDT
void at91_board_set_dtb_name(char *of_name)
{
	strcpy(of_name, CONFIG_DEVICENAME ".dtb");
}
#endif

#define ATMEL_SDHC_GCKDIV_VALUE     3

void at91_sdhc_hw_init(void)
{
#ifdef CONFIG_SDHC0
	const struct pio_desc sdmmc_pins[] = {
#ifdef CONFIG_BOARD_QUIRK_SAM9X75_EB
		{"SDMMC0_CMD",	AT91C_PIN_PA(1), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_A},
		{"SDMMC0_CK",	AT91C_PIN_PA(2), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_A},
		{"SDMMC0_DAT0",	AT91C_PIN_PA(0), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_A},
		{"SDMMC0_DAT1",	AT91C_PIN_PA(3), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_A},
		{"SDMMC0_DAT2",	AT91C_PIN_PA(4), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_A},
		{"SDMMC0_DAT3",	AT91C_PIN_PA(5), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_A},
#else
		{"SDMMC0_CMD",	AT91C_PIN_PA(1), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_CK",	AT91C_PIN_PA(2), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT0",	AT91C_PIN_PA(0), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT1",	AT91C_PIN_PA(3), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT2",	AT91C_PIN_PA(4), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SDMMC0_DAT3",	AT91C_PIN_PA(5), 0, PIO_DEFAULT, PIO_PERIPH_A},
#endif
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};
#endif

#ifdef CONFIG_SDHC1
	const struct pio_desc sdmmc_pins[] = {
#ifdef CONFIG_BOARD_QUIRK_SAM9X75_EB
		{"SDMMC0_CMD",	AT91C_PIN_PA(10), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_B},
		{"SDMMC0_CK",	AT91C_PIN_PA(11), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_B},
		{"SDMMC0_DAT0",	AT91C_PIN_PA(9), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_B},
		{"SDMMC0_DAT1",	AT91C_PIN_PA(6), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_B},
		{"SDMMC0_DAT2",	AT91C_PIN_PA(7), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_B},
		{"SDMMC0_DAT3",	AT91C_PIN_PA(8), 0, PIO_DRVSTR_HI | PIO_SLEWR_CTRL, PIO_PERIPH_B},
#else
		{"SDMMC0_CMD",	AT91C_PIN_PA(10), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"SDMMC0_CK",	AT91C_PIN_PA(11), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"SDMMC0_DAT0",	AT91C_PIN_PA(9), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"SDMMC0_DAT1",	AT91C_PIN_PA(6), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"SDMMC0_DAT2",	AT91C_PIN_PA(7), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"SDMMC0_DAT3",	AT91C_PIN_PA(8), 0, PIO_DEFAULT, PIO_PERIPH_B},
#endif
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};
#endif
	pio_configure(sdmmc_pins);

	pmc_enable_periph_clock(CONFIG_SYS_ID_SDHC, PMC_PERIPH_CLK_DIVIDER_NA);
	pmc_enable_generic_clock(CONFIG_SYS_ID_SDHC,
				 GCK_CSS_PLLADIV2_CLK,
				 ATMEL_SDHC_GCKDIV_VALUE);
}
#endif /* #ifdef CONFIG_SDCARD */

#ifdef CONFIG_NANDFLASH
void nandflash_hw_init(void)
{
	unsigned int reg;
	const struct pio_desc nand_pins[] = {
		{"NANDOE", CONFIG_SYS_NAND_OE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDWE", CONFIG_SYS_NAND_WE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDALE", CONFIG_SYS_NAND_ALE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDCLE", CONFIG_SYS_NAND_CLE_PIN, 0, PIO_DEFAULT,
		 PIO_PERIPH_A},
		{"NANDCS", CONFIG_SYS_NAND_ENABLE_PIN, 1, PIO_DEFAULT,
		 PIO_OUTPUT},
		{"D0", AT91C_PIN_PD(6), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D1", AT91C_PIN_PD(7), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D2", AT91C_PIN_PD(8), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D3", AT91C_PIN_PD(9), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D4", AT91C_PIN_PD(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D5", AT91C_PIN_PD(11), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D6", AT91C_PIN_PD(12), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D7", AT91C_PIN_PD(13), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pio_configure(nand_pins);
	pmc_enable_periph_clock(AT91C_ID_PIOD, PMC_PERIPH_CLK_DIVIDER_NA);
	pmc_enable_periph_clock(AT91C_ID_PMECC, PMC_PERIPH_CLK_DIVIDER_NA);

	reg = readl(AT91C_BASE_SFR + SFR_CCFG_EBICSA);
	reg |= AT91C_EBI_CS2A_SM | AT91C_EBI_NFD0_ON_D16;
	reg &= ~AT91C_EBI_DRV;
	writel(reg, AT91C_BASE_SFR + SFR_CCFG_EBICSA);

	/* Configure SMC CS3 for NAND */
	writel(AT91C_SMC_NWESETUP_(6), AT91C_BASE_SMC + SMC_SETUP2);

	writel(AT91C_SMC_NWEPULSE_(12) | AT91C_SMC_NCS_WRPULSE_(22) |
	       AT91C_SMC_NRDPULSE_(12) | AT91C_SMC_NCS_RDPULSE_(22),
	       AT91C_BASE_SMC + SMC_PULSE2);

	writel(AT91C_SMC_NWECYCLE_(22) | AT91C_SMC_NRDCYCLE_(22),
	       AT91C_BASE_SMC + SMC_CYCLE2);

	writel(AT91C_SMC_READMODE | AT91C_SMC_WRITEMODE | AT91C_SMC_TDFEN |
	       AT91_SMC_TDF_(17), AT91C_BASE_SMC + SMC_CTRL2);
}
#endif /* #ifdef CONFIG_NANDFLASH */
