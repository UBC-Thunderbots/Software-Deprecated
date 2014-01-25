#ifndef STM32LIB_REGISTERS_SYSCFG_H
#define STM32LIB_REGISTERS_SYSCFG_H

/**
 * \file
 *
 * \brief Defines the system configuration controller registers.
 */

#include <stdint.h>

#define SYSCFG_BASE 0x40013800

typedef struct {
	unsigned MEM_MODE : 2;
	unsigned : 30;
} SYSCFG_MEMRMP_t;
#define SYSCFG_MEMRMP (*(volatile SYSCFG_MEMRMP_t *) (SYSCFG_BASE + 0x00))

typedef struct {
	unsigned : 23;
	unsigned MII_RMII_SEL : 1;
	unsigned : 8;
} SYSCFG_PMC_t;
#define SYSCFG_PMC (*(volatile SYSCFG_PMC_t *) (SYSCFG_BASE + 0x04))

typedef uint32_t SYSCFG_EXTICR_t[4];
#define SYSCFG_EXTICR (*(volatile SYSCFG_EXTICR_t *) (SYSCFG_BASE + 0x08))

typedef struct {
	unsigned CMP_PD : 1;
	unsigned : 7;
	unsigned READY : 1;
	unsigned : 23;
} SYSCFG_CMPCR_t;
#define SYSCFG_CMPCR (*(volatile SYSCFG_CMPCR_t *) (SYSCFG_BASE + 0x20))

#endif
