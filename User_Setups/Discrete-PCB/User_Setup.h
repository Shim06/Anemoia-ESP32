#define ST7789_DRIVER // Uncomment if using ST7789 display
// #define ILI9341_DRIVER // Uncomment if using ILI9341 display

#define TFT_WIDTH     240
#define TFT_HEIGHT    320

#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF

#define TFT_MOSI 23
#define TFT_MISO -1
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  -1

#define TOUCH_CS -1

#define LOAD_GLCD
#define LOAD_FONT2

#define SPI_FREQUENCY 80000000