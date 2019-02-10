#include <application.h>

#include <bc_gfx.h>

// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_set_mode(&led, BC_LED_MODE_TOGGLE);
    }

    // Logging in action
    bc_log_info("Button event handler - event: %i", event);
}

uint8_t fifo_get_last_byte(bc_fifo_t *fifo)
{
    uint8_t *ptr = (uint8_t *) fifo->buffer + fifo->head;

    // Decrement one position to get the latest byte in FIFO
    ptr--;

    // Wrap around pointer in cse of underflow
    if (ptr < (uint8_t*)fifo->buffer)
    {
        ptr += fifo->size;
    }

    return *ptr;

}

bc_fifo_t tx_fifo;
bc_fifo_t rx_fifo;
uint8_t tx_fifo_buffer[64];
uint8_t rx_fifo_buffer[4096];

// This should be optimized to use less buffers or to write base64 decoded data directly by words to the DMA LCD buffer
uint8_t rx_framebuffer[4096];

uint8_t lcd_framebuffer[2048];

void uart_handler(bc_uart_channel_t channel, bc_uart_event_t event, void *param)
{

    if (event == BC_UART_EVENT_ASYNC_WRITE_DONE)
    {
        // here you can for example send more data
    }
    if (event == BC_UART_EVENT_ASYNC_READ_DATA)
    {
        uint8_t last_byte = fifo_get_last_byte(&rx_fifo);
        if (last_byte  == 0x0D)
        {

            bc_uart_write(BC_UART_UART2, "1", 1);

            // Read data from FIFO by a single byte as you receive it
            // 2.2ms
            size_t number_of_rx_bytes = bc_uart_async_read(BC_UART_UART2, rx_framebuffer, sizeof(rx_framebuffer));
            char uart_tx[32];

            bc_uart_write(BC_UART_UART2, "2", 1);

            // 7.2 ms
            // decode, minus 1 byte CR
            size_t output_len = sizeof(lcd_framebuffer);
            volatile bool ret = bc_base64_decode(lcd_framebuffer, &output_len, (char*)rx_framebuffer, number_of_rx_bytes - 1);

            //bc_uart_write(BC_UART_UART2, "3", 1);

            //snprintf(uart_tx, sizeof(uart_tx), "RX: %d, b64_ok: %d\r\n", number_of_rx_bytes, ret);
            //bc_uart_async_write(BC_UART_UART2, uart_tx, strlen(uart_tx));

            bc_image_t img;
            img.data = lcd_framebuffer;
            img.width = 128;
            img.height = 128;

            //bc_uart_write(BC_UART_UART2, "4", 1);
            // 0.6ms
            //bc_module_lcd_clear();

            bc_uart_write(BC_UART_UART2, "5", 1);

            // 62 ms!
            //bc_module_lcd_draw_image(0, 0, &img);

/*
            // Skip mode byte + addr byte
            uint32_t byteIndex = 2;
            // Skip lines
            byteIndex += y * 18;
            // Select column byte
            byteIndex += x / 8;

            uint8_t bitMask = 1 << (7 - (x % 8));


            self->_framebuffer[byteIndex] |= bitMask;*/

            // 7ms
            bc_gfx_t *gfx = bc_module_lcd_get_gfx();
            uint8_t *framebufferStart = &((bc_ls013b7dh03_t*)gfx->_display)->_framebuffer[2];

            for(int i = 0; i < 128; i++)
            {
                //
                uint8_t *framebuffer = framebufferStart + i * 18;
                //
                memcpy(framebuffer, &lcd_framebuffer[i * 16], 16);
            }

            //bc_ls013b7dh03_t
            //_bc_module_lcd.ls013b7dh03._framebuffer[10] = 0;



            bc_uart_write(BC_UART_UART2, "6", 1);

            bc_module_lcd_update();

            bc_uart_write(BC_UART_UART2, "7", 1);
        }
    }
    if (event == BC_UART_EVENT_ASYNC_READ_TIMEOUT)
    {
        // No data received during set timeout period
        //char uart_tx[] = "Timeout\r\n";
        //bc_uart_async_write(BC_UART_UART2, uart_tx, strlen(uart_tx));
        // You can also read received bytes here instead of BC_UART_EVENT_ASYNC_READ_DATA
    }
}

void application_init(void)
{
    bc_system_pll_enable();

    // Initialize logging
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize LCD
    // The parameter is internal buffer in SDK, no need to define it
    bc_module_lcd_init();

    // Init default font, this is necessary
    // See other fonts in sdk/bcl/inc/bc_font_common.h
    bc_module_lcd_set_font(&bc_font_ubuntu_15);

    // Draw string at X, Y location
    bc_module_lcd_draw_string(10, 5, "Hello world!", true);

    // Don't forget to update
    bc_module_lcd_update();

    bc_uart_init(BC_UART_UART2, BC_UART_BAUDRATE_921600, BC_UART_SETTING_8N1);
    bc_uart_set_event_handler(BC_UART_UART2, uart_handler, NULL);

    bc_fifo_init(&tx_fifo, tx_fifo_buffer, sizeof(tx_fifo_buffer));
    bc_fifo_init(&rx_fifo, rx_fifo_buffer, sizeof(rx_fifo_buffer));

    bc_uart_set_async_fifo(BC_UART_UART2, &tx_fifo, &rx_fifo);

    bc_uart_async_read_start(BC_UART_UART2, 500);
}
/*
void application_task(void)
{
    // Logging in action
    bc_log_debug("application_task run");

    // Plan next run this function after 1000 ms
    bc_scheduler_plan_current_from_now(1000);
}*/
