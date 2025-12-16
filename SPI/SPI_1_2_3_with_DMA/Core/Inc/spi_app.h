/**
  ******************************************************************************
  * @file           : spi_app.h
  * @author         : Luca Cassi
  ******************************************************************************
*/


#ifndef INC_SPI_APP_H_
#define INC_SPI_APP_H_

#include <stdint.h>
#include "stm32h7xx_hal.h"

/* Generic SPI device handle (SPI + manual CS) */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *CS_Port;
    uint16_t CS_Pin;

    uint8_t cs_active_low;     // 1: CS active low (most common), 0: CS active high

    /* DMA state (optional use) */
    volatile uint8_t dma_done; // 0 running, 1 done
    volatile uint8_t dma_err;  // 0 ok, 1 error
    volatile uint32_t dma_hal_error;

    /* Optional: store last transfer length for cache maintenance */
    uint16_t last_len;
    uint8_t *last_rx;
    const uint8_t *last_tx;

} SPI_APP_Device;

/* Functions */
void spi_app_initStruct(SPI_APP_Device *dev,
                        SPI_HandleTypeDef *hspi,
                        GPIO_TypeDef *CS_Port,
                        uint16_t CS_Pin,
                        uint8_t cs_active_low);

void spi_app_cs_low(SPI_APP_Device *dev);
void spi_app_cs_high(SPI_APP_Device *dev);

/* Blocking (polling) helpers */
HAL_StatusTypeDef spi_app_write(SPI_APP_Device *dev, uint8_t *tx, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef spi_app_read(SPI_APP_Device *dev, uint8_t *rx, uint16_t len, uint32_t timeout);
HAL_StatusTypeDef spi_app_transfer(SPI_APP_Device *dev, uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout);

/* DMA helpers (async) - requires SPI configured with DMA in CubeMX */
HAL_StatusTypeDef spi_app_transfer_dma(SPI_APP_Device *dev, uint8_t *tx, uint8_t *rx, uint16_t len);
HAL_StatusTypeDef spi_app_write_dma(SPI_APP_Device *dev, uint8_t *tx, uint16_t len);
HAL_StatusTypeDef spi_app_read_dma(SPI_APP_Device *dev, uint8_t *rx, uint16_t len);

/* To be called from HAL callbacks */
void spi_app_dma_cplt(SPI_APP_Device *dev);
void spi_app_dma_error(SPI_APP_Device *dev);

/* STM32H7 cache helpers (Placeholders: maybe not needed) */
void spi_app_cache_clean(const void *addr, uint32_t size);
void spi_app_cache_invalidate(void *addr, uint32_t size);

#endif /* INC_SPI_APP_H_ */
