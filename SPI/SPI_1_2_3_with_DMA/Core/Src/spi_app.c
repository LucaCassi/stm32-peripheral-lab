/**
  ******************************************************************************
  * @file           : spi_app.c
  * @author         : Luca Cassi
  ******************************************************************************
*/

/* Includes */
#include "spi_app.h"
#include <string.h>

/* Local dummy TX buffer used to clock RX-only operations */
static uint8_t spi_app_dummy_tx[256];

/* Functions */
void spi_app_initStruct(SPI_APP_Device *dev,
                        SPI_HandleTypeDef *hspi,
                        GPIO_TypeDef *CS_Port,
                        uint16_t CS_Pin,
                        uint8_t cs_active_low)
{
    dev->hspi = hspi;
    dev->CS_Port = CS_Port;
    dev->CS_Pin = CS_Pin;
    dev->cs_active_low = cs_active_low;

    dev->dma_done = 1;
    dev->dma_err = 0;
    dev->dma_hal_error = 0;

    dev->last_len = 0;
    dev->last_rx = 0;
    dev->last_tx = 0;

    memset(spi_app_dummy_tx, 0xFF, sizeof(spi_app_dummy_tx));

    spi_app_cs_high(dev);
}

void spi_app_cs_low(SPI_APP_Device *dev)
{
    if (dev->cs_active_low)
        HAL_GPIO_WritePin(dev->CS_Port, dev->CS_Pin, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(dev->CS_Port, dev->CS_Pin, GPIO_PIN_SET);
}

void spi_app_cs_high(SPI_APP_Device *dev)
{
    if (dev->cs_active_low)
        HAL_GPIO_WritePin(dev->CS_Port, dev->CS_Pin, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(dev->CS_Port, dev->CS_Pin, GPIO_PIN_RESET);
}

HAL_StatusTypeDef spi_app_write(SPI_APP_Device *dev, uint8_t *tx, uint16_t len, uint32_t timeout)
{
    HAL_StatusTypeDef ret;

    spi_app_cs_low(dev);
    ret = HAL_SPI_Transmit(dev->hspi, tx, len, timeout);
    spi_app_cs_high(dev);

    return ret;
}

HAL_StatusTypeDef spi_app_read(SPI_APP_Device *dev, uint8_t *rx, uint16_t len, uint32_t timeout)
{
    HAL_StatusTypeDef ret = HAL_OK;
    uint16_t remaining = len;
    uint16_t offset = 0;

    spi_app_cs_low(dev);

    while (remaining > 0 && ret == HAL_OK)
    {
        uint16_t chunk;
        if (remaining > sizeof(spi_app_dummy_tx)) chunk = sizeof(spi_app_dummy_tx);
        else chunk = remaining;
        ret = HAL_SPI_TransmitReceive(dev->hspi, spi_app_dummy_tx, &rx[offset], chunk, timeout);
        offset += chunk;
        remaining -= chunk;
    }

    spi_app_cs_high(dev);
    return ret;
}

HAL_StatusTypeDef spi_app_transfer(SPI_APP_Device *dev, uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout)
{
    HAL_StatusTypeDef ret;

    spi_app_cs_low(dev);
    ret = HAL_SPI_TransmitReceive(dev->hspi, tx, rx, len, timeout);
    spi_app_cs_high(dev);

    return ret;
}

HAL_StatusTypeDef spi_app_transfer_dma(SPI_APP_Device *dev, uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef ret;

    dev->dma_done = 0;
    dev->dma_err = 0;
    dev->dma_hal_error = 0;

    dev->last_len = len;
    dev->last_tx = tx;
    dev->last_rx = rx;

    if (tx) spi_app_cache_clean(tx, len);

    spi_app_cs_low(dev);
    ret = HAL_SPI_TransmitReceive_DMA(dev->hspi, tx, rx, len);

    if (ret != HAL_OK)
    {
        spi_app_cs_high(dev);
        dev->dma_done = 1;
        dev->dma_err = 1;
        dev->dma_hal_error = dev->hspi->ErrorCode;
    }

    return ret;
}

HAL_StatusTypeDef spi_app_write_dma(SPI_APP_Device *dev, uint8_t *tx, uint16_t len)
{
    HAL_StatusTypeDef ret;

    dev->dma_done = 0;
    dev->dma_err = 0;
    dev->dma_hal_error = 0;

    dev->last_len = len;
    dev->last_tx = tx;
    dev->last_rx = 0;

    spi_app_cache_clean(tx, len);

    spi_app_cs_low(dev);
    ret = HAL_SPI_Transmit_DMA(dev->hspi, tx, len);

    if (ret != HAL_OK)
    {
        spi_app_cs_high(dev);
        dev->dma_done = 1;
        dev->dma_err = 1;
        dev->dma_hal_error = dev->hspi->ErrorCode;
    }

    return ret;
}

HAL_StatusTypeDef spi_app_read_dma(SPI_APP_Device *dev, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef ret;

    if (len > sizeof(spi_app_dummy_tx))
        return HAL_ERROR;

    dev->dma_done = 0;
    dev->dma_err = 0;
    dev->dma_hal_error = 0;

    dev->last_len = len;
    dev->last_tx = spi_app_dummy_tx;
    dev->last_rx = rx;

    spi_app_cs_low(dev);
    ret = HAL_SPI_TransmitReceive_DMA(dev->hspi, spi_app_dummy_tx, rx, len);

    if (ret != HAL_OK)
    {
        spi_app_cs_high(dev);
        dev->dma_done = 1;
        dev->dma_err = 1;
        dev->dma_hal_error = dev->hspi->ErrorCode;
    }

    return ret;
}

/* Call this from HAL_SPI_TxRxCpltCallback / TxCplt / RxCplt */
void spi_app_dma_cplt(SPI_APP_Device *dev)
{
    if (dev->last_rx)
        spi_app_cache_invalidate(dev->last_rx, dev->last_len);

    spi_app_cs_high(dev);

    dev->dma_done = 1;
    dev->dma_err = 0;
    dev->dma_hal_error = 0;
}

/* Call this from HAL_SPI_ErrorCallback */
void spi_app_dma_error(SPI_APP_Device *dev)
{
    spi_app_cs_high(dev);

    dev->dma_done = 1;
    dev->dma_err = 1;
    dev->dma_hal_error = dev->hspi->ErrorCode;
}

/* Cache helpers (STM32H7) */
void spi_app_cache_clean(const void *addr, uint32_t size)
{
    /* Placeholder: On STM32H7 this may be required before DMA TX to ensure data coherency with D-Cache. I do not implement this */
    (void)addr;
    (void)size;
}

void spi_app_cache_invalidate(void *addr, uint32_t size)
{
    /* Placeholder: On STM32H7 this may be required before DMA TX to ensure data coherency with D-Cache. I do not implement this */
    (void)addr;
    (void)size;
}
