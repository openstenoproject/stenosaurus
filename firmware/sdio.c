// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
//
// This library is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this library.  If not, see <http://www.gnu.org/licenses/>.
//
// This file implements the SDIO interface to the Stenosaurus. See the header
// file for interface documentation to this code.

#include <stdbool.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/sdio.h>
#include <libopencm3/stm32/dma.h>
#include "sdio.h"
#include "clock.h"

static struct {
    // Relative Card Address, used when sending certain commands.
    uint16_t rca;
    // If ccs true then the card is a SDHC/SDXC card and uses block based
    // addressing. Otherwise it is an SDSC card and uses byte based addresing.
    bool ccs;
    // Size is in 512 byte blocks. Max size is 2T or 0x100000000.
    uint32_t size;
} sd_card_info;

static void clear_card_info(void) {
    sd_card_info.rca = 0;
    sd_card_info.ccs = false;
    sd_card_info.size = 0;
}

// Set up the GPIO pins and peripheral clocks for the SDIO peripheral.
void sdio_init(void) {
    // Enable the clock for SDIO.
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_SDIOEN);
    // Enable the clock for DMA2, which is the one connected to the SDIO.
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_DMA2EN);

    // Enable the clock for all the IO ports used by SDIO and the card detect
    // pin.
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPDEN);

    // Pin mappings for STM32F103.
    // PC8 - SDIO_D0
    // PC9 - SDIO_D1
    // PC10 - SDIO_D2
    // PC11 - SDIO_D3
    // PC12 - SDIO_CK
    // PD2 - SDIO_CMD

    // The spec says to configure all the pins as Alternate Function Push/Pull.
    // Since standard speed is up to 25Mhz, the lowest compatible setting is
    // 50Mhz since the only other options I have are 2Mhz and 10Mhz, which is
    // too low.
    gpio_set_mode(GPIOC,
                  GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12);
    gpio_set_mode(GPIOD,
                  GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO2);

    // PA8 is pulled low on the WaveShare board when a card is present.
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO8);
    // Set the output register for GPIOA pin8, which when configured as input
    // with pullup/down, enables the pull-up resistor (resetting the  pin, which
    // is the default, enables the pull down resistor instead).
    gpio_set(GPIOA, GPIO8);
}

// Send a command on the SDIO bus.
void sdio_send_command(uint32_t cmd, uint32_t arg) {
    cmd &= SDIO_CMD_CMDINDEX_MSK;
    uint32_t waitresp = SDIO_CMD_WAITRESP_SHORT;
    if (cmd == 0) {
        waitresp = SDIO_CMD_WAITRESP_NO_0;
    } else if (cmd == 2 || cmd == 9 || cmd == 10) {
        waitresp = SDIO_CMD_WAITRESP_LONG;
    }

    /* If a data transaction is in progress, wait for it to finish */
#if 0
    if ((cmd != 12) && (SDIO_STA & (SDIO_STA_RXACT | SDIO_STA_TXACT))) {
        // XXX: This should be an error, we don't have multithread
        tmp_val |= SDIO_CMD_WAITPEND;
    }
#endif

    SDIO_ICR = 0x7ff; // Reset all signals we use (and some we don't).
    SDIO_ARG = arg; // The arg must be set before the command.
    // Set the command and associated bits.
    SDIO_CMD = (cmd | SDIO_CMD_CPSMEN | waitresp);
}

sdio_error_t get_command_result(void) {
    uint32_t status = SDIO_STA & 0xFFF;

    if (status & SDIO_STA_CMDACT) return SDIO_EINPROGRESS;

    if (status & SDIO_STA_CMDREND) {
        SDIO_ICR = SDIO_STA_CMDREND;
        return SDIO_ESUCCESS;
    }

    if (status & SDIO_STA_CMDSENT) {
        SDIO_ICR = SDIO_STA_CMDSENT;
        return SDIO_ESUCCESS;
    }

    if (status & SDIO_STA_CTIMEOUT) {
        SDIO_ICR = SDIO_STA_CTIMEOUT;
        return SDIO_ECTIMEOUT;
    }

    if (status & SDIO_STA_CCRCFAIL) {
        SDIO_ICR = SDIO_STA_CCRCFAIL;
        return SDIO_ECCRCFAIL;
    }

    return SDIO_EUNKNOWN;
}

void sdio_power_up(void) {
    SDIO_POWER = SDIO_POWER_PWRCTRL_PWRON;
    while (SDIO_POWER != SDIO_POWER_PWRCTRL_PWRON);
    SDIO_CLKCR = SDIO_CLKCR_CLKEN | 118;
}

void sdio_power_down(void) {
    SDIO_POWER = SDIO_POWER_PWRCTRL_PWROFF;
}

static sdio_error_t send_command_wait(uint32_t cmd, uint32_t arg) {
    sdio_error_t result;
    sdio_send_command(cmd, arg);
    while ((result = get_command_result()) == SDIO_EINPROGRESS);
    return result;
}

static const int max_retries = 5;

static sdio_error_t send_command_retry(uint32_t cmd, uint32_t arg) {
    sdio_error_t result;
    for (int i = 0; i < max_retries; ++i) {
        result = send_command_wait(cmd, arg);
        if (result == SDIO_ESUCCESS) {
            break;
        }
    }
    return result;
}

static sdio_error_t sdio_select(void) {
    return send_command_retry(7, sd_card_info.rca << 16);
}

static sdio_error_t send_application_command(uint32_t cmd, uint32_t arg) {
    sdio_error_t result;
    sdio_error_t expected_result = SDIO_ESUCCESS;
    if (cmd == 41) expected_result = SDIO_ECCRCFAIL;
    for (int i = 0; i < max_retries; ++i) {
        result = send_command_wait(55, sd_card_info.rca << 16);
        if (result != SDIO_ESUCCESS) {
            continue;
        }
        result = send_command_wait(cmd, arg);
        if (result == expected_result) {
            break;
        }
    }
    return result;
}

bool sdio_card_init(void) {
    clear_card_info();

    bool card_present = (GPIOA_IDR & GPIO8) == 0;
    if (!card_present) return 1;

    sdio_power_up();

    sdio_error_t result;
    uint32_t response;

    if (send_command_retry(0, 0) != SDIO_ESUCCESS) {
        sdio_power_down();
        return false;
    }

    bool hcs;
    result = send_command_retry(8, 0x1F1);
    if (result == SDIO_ESUCCESS && SDIO_RESP1 == 0x1F1) {
        hcs = true;
    } else if (result == SDIO_ECTIMEOUT) {
        hcs = false;
    } else {
        sdio_power_down();
        return false;
    }

    const uint32_t OCR_BUSY = 0x80000000;
    const uint32_t OCR_HCS = 0x40000000;
    const uint32_t OCR_CCS = 0x40000000;

    bool acmd41_success = false;
    uint32_t deadline = system_millis + 1000;
    while (system_millis < deadline) {
        result = send_application_command(41, 0x100000 | (hcs ? OCR_HCS : 0));
        response = SDIO_RESP1;
        if (result == SDIO_ECCRCFAIL && (response & OCR_BUSY) != 0) {
            sd_card_info.ccs = (response & OCR_CCS) != 0;
            acmd41_success = true;
            break;
        }
    }
    if (!acmd41_success) {
        sdio_power_down();
        return false;
    }

    if (send_command_retry(2, 0) != SDIO_ESUCCESS) {
        sdio_power_down();
        return false;
    }

    bool cmd3_success = false;
    for (int i = 0; i < max_retries; ++i) {
        if (send_command_wait(3, 0) == SDIO_ESUCCESS) {
            response = SDIO_RESP1;
            sd_card_info.rca = response >> 16;
            if (sd_card_info.rca != 0) {
                cmd3_success = true;
                break;
            }
        }
    }
    if (!cmd3_success) {
        sdio_power_down();
        return false;
    }

    if (send_command_retry(9, sd_card_info.rca << 16) != SDIO_ESUCCESS) {
        sdio_power_down();
        return false;
    }

    // Get the size of the card from the CSD. There are two versions.
    // For V1: byte_size = BLOCK_LEN * MULT * (C_SIZE + 1) bytes.
    // For V2: byte_size = (C_SIZE + 1) * 512K bytes.
    // We address the card in 512 byte blocks so we set size = byte_size / 512.
    uint32_t csd_version = SDIO_RESP1 >> 30;
    if (csd_version == 0) {
        // Until I find an old card, this is untested.
        uint32_t read_bl_len = (SDIO_RESP2 >> 16) & 0xF;
        uint32_t c_size = ((SDIO_RESP2 & 0x3FF) << 2) | (SDIO_RESP3 >> 30);
        uint32_t c_size_mult = (SDIO_RESP3 >> 15) & 0x7;
        uint32_t mult = 1 << (c_size_mult + 2);
        uint32_t blocknr = (c_size + 1) * mult;
        uint32_t block_len = 1 << read_bl_len;
        sd_card_info.size = (block_len * blocknr) >> 9;
    } else if (csd_version == 1) {
        uint32_t c_size = ((SDIO_RESP2 & 0x3F) << 16) | (SDIO_RESP3 >> 16);
        // (c_size + 1) * 512K / 512 = (c_size + 1) * 1024 = (c_size + 1) << 10.
        sd_card_info.size = (c_size + 1) << 10;
    } else {
        sdio_power_down();
        return false;
    }

    if (sdio_select() != SDIO_ESUCCESS) {
        sdio_power_down();
        return false;
    }

    // Tell the card to use a 4-width bus.
    if (send_application_command(6, 2) != SDIO_ESUCCESS) {
        sdio_power_down();
        return false;
    }

    // Set our side of the bus to be 4 wide and speed up the clock to 24Mhz.
    SDIO_CLKCR = SDIO_CLKCR_CLKEN | SDIO_CLKCR_WIDBUS_4;

    return true;
}

bool wait_for_data_ready(void) {
    uint32_t timeout = system_millis + 1000;
    while (system_millis < timeout) {
        if (send_command_wait(13, sd_card_info.rca << 16) == SDIO_ESUCCESS &&
                (SDIO_RESP1 & 0x100) != 0) {
            return true;
        }
    }
    return false;
}

bool sdio_read_block(uint32_t address, uint32_t *buffer) {
    if (!wait_for_data_ready()) {
        return false;
    }

    if (!sd_card_info.ccs) {
        address *= 512;

        if (send_command_retry(16, 512) != SDIO_ESUCCESS) {
            return false;
        }
    }

    dma_channel_reset(DMA2, DMA_CHANNEL4);
    dma_set_memory_size(DMA2, DMA_CHANNEL4, DMA_CCR_MSIZE_32BIT);
    dma_set_peripheral_size(DMA2, DMA_CHANNEL4, DMA_CCR_PSIZE_32BIT);
    dma_enable_memory_increment_mode(DMA2, DMA_CHANNEL4);
    dma_disable_peripheral_increment_mode(DMA2, DMA_CHANNEL4);
    dma_set_read_from_peripheral(DMA2, DMA_CHANNEL4);
    dma_set_peripheral_address(DMA2, DMA_CHANNEL4, (uint32_t)&SDIO_FIFO);
    dma_set_memory_address(DMA2, DMA_CHANNEL4, (uint32_t)buffer);
    dma_set_number_of_data(DMA2, DMA_CHANNEL4, 128);
    dma_enable_channel(DMA2, DMA_CHANNEL4);

    // A 100ms timeout expressed as ticks in the 24Mhz bus clock.
    SDIO_DTIMER = 2400000;
    // These two registers must be set before SDIO_DCTRL.
    SDIO_DLEN = 512;
    SDIO_DCTRL = SDIO_DCTRL_DBLOCKSIZE_9 | SDIO_DCTRL_DMAEN |
                 SDIO_DCTRL_DTDIR | SDIO_DCTRL_DTEN;

    if (send_command_wait(17, 0) != SDIO_ESUCCESS) {
        return false;
    }

    const uint32_t DATA_RX_ERROR_FLAGS = (SDIO_STA_STBITERR |
                                          SDIO_STA_RXOVERR |
                                          SDIO_STA_DTIMEOUT |
                                          SDIO_STA_DCRCFAIL);
    const uint32_t DATA_RX_SUCCESS_FLAGS = (SDIO_STA_DBCKEND |
                                            SDIO_STA_DATAEND);

    while (true) {
        uint32_t result = SDIO_STA;
        if (result & (DATA_RX_SUCCESS_FLAGS | DATA_RX_ERROR_FLAGS)) {
            if (result & DATA_RX_ERROR_FLAGS) {
                return false;
            }
            break;
        }
    }

    return true;
}

bool sdio_write_block(uint32_t address, uint32_t *buffer) {
    if (!wait_for_data_ready()) {
        return false;
    }

    if (!sd_card_info.ccs) {
        address *= 512;

        if (send_command_retry(16, 512) != SDIO_ESUCCESS) {
            return false;
        }
    }

    dma_channel_reset(DMA2, DMA_CHANNEL4);
    dma_set_memory_size(DMA2, DMA_CHANNEL4, DMA_CCR_MSIZE_32BIT);
    dma_set_peripheral_size(DMA2, DMA_CHANNEL4, DMA_CCR_PSIZE_32BIT);
    dma_enable_memory_increment_mode(DMA2, DMA_CHANNEL4);
    dma_disable_peripheral_increment_mode(DMA2, DMA_CHANNEL4);
    dma_set_read_from_memory(DMA2, DMA_CHANNEL4);
    dma_set_peripheral_address(DMA2, DMA_CHANNEL4, (uint32_t)&SDIO_FIFO);
    dma_set_memory_address(DMA2, DMA_CHANNEL4, (uint32_t)buffer);
    dma_set_number_of_data(DMA2, DMA_CHANNEL4, 128);
    dma_enable_channel(DMA2, DMA_CHANNEL4);

    if (send_command_wait(24, 0) != SDIO_ESUCCESS) {
        return false;
    }

    // A 500ms timeout expressed as ticks in the 24Mhz bus clock.
    SDIO_DTIMER = 12000000;
    // These two registers must be set before SDIO_DCTRL.
    SDIO_DLEN = 512;
    SDIO_DCTRL = SDIO_DCTRL_DBLOCKSIZE_9 | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;

    const uint32_t DATA_TX_ERROR_FLAGS = (SDIO_STA_STBITERR |
                                          SDIO_STA_TXUNDERR |
                                          SDIO_STA_DTIMEOUT |
                                          SDIO_STA_DCRCFAIL);
    const uint32_t DATA_TX_SUCCESS_FLAGS = (SDIO_STA_DBCKEND |
                                            SDIO_STA_DATAEND);

    while (true) {
        uint32_t result = SDIO_STA;
        if (result & (DATA_TX_SUCCESS_FLAGS | DATA_TX_ERROR_FLAGS)) {
            if (result & DATA_TX_ERROR_FLAGS) {
                return false;
            }
            break;
        }
    }

    return true;
}
