#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "JPEGDEC.h"
#include "image_data.h"

#define TFT_SPI_PORT spi0
#define TFT_PIN_SCK 2
#define TFT_PIN_MOSI 3
#define TFT_PIN_CS 1
#define TFT_PIN_DC 0
#define TFT_PIN_RST 26
#define TFT_PIN_BL 27

#define TFT_WIDTH 240
#define TFT_HEIGHT 240

JPEGDEC jpeg;

void tft_write_command(uint8_t cmd)
{
    gpio_put(TFT_PIN_DC, 0);
    gpio_put(TFT_PIN_CS, 0);
    spi_write_blocking(TFT_SPI_PORT, &cmd, 1);
    gpio_put(TFT_PIN_CS, 1);
}

void tft_write_data(uint8_t *data, size_t len)
{
    gpio_put(TFT_PIN_DC, 1);
    gpio_put(TFT_PIN_CS, 0);
    spi_write_blocking(TFT_SPI_PORT, data, len);
    gpio_put(TFT_PIN_CS, 1);
}

void tft_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint8_t caset[] = {0, (uint8_t)x, 0, (uint8_t)(x + w - 1)};
    uint8_t raset[] = {0, (uint8_t)y, 0, (uint8_t)(y + h - 1)};

    tft_write_command(0x2A);
    tft_write_data(caset, 4);
    tft_write_command(0x2B);
    tft_write_data(raset, 4);
    tft_write_command(0x2C);
}

void tft_init()
{
    spi_init(TFT_SPI_PORT, 62500000);
    gpio_set_function(TFT_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(TFT_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(TFT_PIN_CS);
    gpio_set_dir(TFT_PIN_CS, GPIO_OUT);
    gpio_put(TFT_PIN_CS, 1);
    gpio_init(TFT_PIN_DC);
    gpio_set_dir(TFT_PIN_DC, GPIO_OUT);
    gpio_put(TFT_PIN_DC, 1);

    if (TFT_PIN_RST != -1)
    {
        gpio_init(TFT_PIN_RST);
        gpio_set_dir(TFT_PIN_RST, GPIO_OUT);
        gpio_put(TFT_PIN_RST, 1);
        sleep_ms(100);
        gpio_put(TFT_PIN_RST, 0);
        sleep_ms(100);
        gpio_put(TFT_PIN_RST, 1);
        sleep_ms(200);
    }

    if (TFT_PIN_BL != -1)
    {
        gpio_init(TFT_PIN_BL);
        gpio_set_dir(TFT_PIN_BL, GPIO_OUT);
        gpio_put(TFT_PIN_BL, 1);
    }

    tft_write_command(0x11);
    sleep_ms(120);
    tft_write_command(0x3A);
    uint8_t color[] = {0x55};
    tft_write_data(color, 1);
    tft_write_command(0x36);
    uint8_t rot[] = {0x00};
    tft_write_data(rot, 1);
    tft_write_command(0x21);
    tft_write_command(0x29);
}

int drawMCU(JPEGDRAW *pDraw)
{

    tft_set_window(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);

    gpio_put(TFT_PIN_DC, 1);
    gpio_put(TFT_PIN_CS, 0);

    spi_write_blocking(TFT_SPI_PORT, (uint8_t *)pDraw->pPixels, pDraw->iWidth * pDraw->iHeight * 2);

    gpio_put(TFT_PIN_CS, 1);
    return 1;
}

int main()
{
    stdio_init_all();

    gpio_init(0);
    gpio_set_dir(0, GPIO_OUT);
    gpio_put(0, 1);

    tft_init();

    tft_set_window(0, 0, TFT_WIDTH, TFT_HEIGHT);

    while (true)
    {

        for (int i = 0; i < total_images; i++)
        {

            if (jpeg.openFLASH((uint8_t *)image_list[i], image_sizes[i], drawMCU))
            {

                jpeg.setPixelType(RGB565_BIG_ENDIAN);
                jpeg.decode(0, 0, 0);
                jpeg.close();
            }

            sleep_ms(3000);
        }
    }
}