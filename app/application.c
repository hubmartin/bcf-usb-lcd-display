#include <application.h>
#include <bc_apds9960.h>
//#include <bc_gfx.h>

// LED instance
bc_led_t led;

bc_led_t led_lcd_red;
bc_led_t led_lcd_green;
bc_led_t led_lcd_blue;

bc_lis2dh12_t lis2dh12;
bc_dice_t dice;
bc_dice_face_t face = BC_DICE_FACE_UNKNOWN;
bc_module_lcd_rotation_t rotation = BC_MODULE_LCD_ROTATION_0;

bc_apds9960_t apds9960;

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

bc_led_t* get_led_by_letter(char ch)
{
    switch(ch)
    {
        case 'R':
            return &led_lcd_red;
            break;

        case 'G':
            return &led_lcd_green;
            break;

        case 'B':
            return &led_lcd_blue;
            break;

        default:
            return &led_lcd_blue;
            break;

    }
}

void uart_handler(bc_uart_channel_t channel, bc_uart_event_t event, void *param)
{

    if (event == BC_UART_EVENT_ASYNC_WRITE_DONE)
    {
        // here you can for example send more data
    }
    if (event == BC_UART_EVENT_ASYNC_READ_DATA)
    {
        uint8_t last_byte = fifo_get_last_byte(&rx_fifo);
        // CR, LF 0x0D, 0x0A
        if (last_byte  == 0x0A)
        {

            //bc_uart_write(BC_UART_UART2, "1", 1);

            // Read data from FIFO by a single byte as you receive it
            // 2.2ms
            size_t number_of_rx_bytes = bc_uart_async_read(BC_UART_UART2, rx_framebuffer, sizeof(rx_framebuffer));
            //char uart_tx[32];
/*
            if (number_of_rx_bytes == 3)
            {
                // Pulse
                if (rx_framebuffer[0] == 'P')
                {
                    bc_led_pulse(get_led_by_letter(rx_framebuffer[1]), 1000);
                }
                // On
                if (rx_framebuffer[0] == 'O')
                {
                    bc_led_set_mode(get_led_by_letter(rx_framebuffer[1]), BC_LED_MODE_ON);
                }
                // Off
                if (rx_framebuffer[0] == 'F')
                {
                    bc_led_set_mode(get_led_by_letter(rx_framebuffer[1]), BC_LED_MODE_OFF);
                }
                // Blink
                if (rx_framebuffer[0] == 'B')
                {
                    bc_led_set_mode(get_led_by_letter(rx_framebuffer[1]), BC_LED_MODE_BLINK);
                }


                return;
            }*/

            //bc_uart_write(BC_UART_UART2, "2", 1);

            // 7.2 ms
            // decode, minus 2 byte CR + LF
            size_t output_len = sizeof(lcd_framebuffer);
            volatile bool ret = bc_base64_decode(lcd_framebuffer, &output_len, (char*)rx_framebuffer, number_of_rx_bytes - 2);
            (void) ret;

            //bc_uart_write(BC_UART_UART2, "3", 1);

            //snprintf(uart_tx, sizeof(uart_tx), "RX: %d, b64_ok: %d\r\n", number_of_rx_bytes, ret);
            //bc_uart_async_write(BC_UART_UART2, uart_tx, strlen(uart_tx));

            // img.data = lcd_framebuffer;
            // img.width = 128;
            // img.height = 128;

            //bc_uart_write(BC_UART_UART2, "4", 1);
            // 0.6ms
            //bc_module_lcd_clear();

            //bc_uart_write(BC_UART_UART2, "5", 1);

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



            //bc_uart_write(BC_UART_UART2, "6", 1);

            bc_module_lcd_update();

            //bc_uart_write(BC_UART_UART2, "7", 1);
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

void lis2dh12_event_handler(bc_lis2dh12_t *self, bc_lis2dh12_event_t event, void *event_param)
{
    (void) event_param;

    if (event == BC_LIS2DH12_EVENT_UPDATE)
    {
        bc_lis2dh12_result_g_t result;

        bc_lis2dh12_get_result_g(self, &result);

        bc_dice_feed_vectors(&dice, result.x_axis, result.y_axis, result.z_axis);

        face = bc_dice_get_face(&dice);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "accelerometer:%d\n", face);
        bc_uart_async_write(BC_UART_UART2, buffer, strlen(buffer));
    }
}

void lcd_button_left_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

    if (event == BC_BUTTON_EVENT_CLICK)
	{
        bc_led_pulse(&led_lcd_blue, 30);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "button:left\n");
        bc_uart_async_write(BC_UART_UART2, buffer, strlen(buffer));
    }
}

void lcd_button_right_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

	if (event == BC_BUTTON_EVENT_CLICK)
	{
        bc_led_pulse(&led_lcd_red, 30);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "button:right\n");
        bc_uart_async_write(BC_UART_UART2, buffer, strlen(buffer));
    }
}

void apds9960_event_handler(bc_apds9960_t *self, bc_apds9960_event_t event, void *event_param)
{
    if (event == BC_APDS9960_EVENT_UPDATE)
    {
        bc_apds9960_gesture_t gesture;
        bc_apds9960_get_gesture(self, &gesture);
        char buffer[64];
        char *p[] = {
            "none",
            "left",
            "right",
            "up",
            "down",
            "near",
            "far",
            "all"
        };

        snprintf(buffer, sizeof(buffer), "gesture:%s\n", p[gesture]);
        bc_uart_async_write(BC_UART_UART2, buffer, strlen(buffer));

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

    // Initialize LCD
    // The parameter is internal buffer in SDK, no need to define it
    bc_module_lcd_init();

    // Init default font, this is necessary
    // See other fonts in sdk/bcl/inc/bc_font_common.h
    bc_module_lcd_set_font(&bc_font_ubuntu_15);

    // Draw string at X, Y location
    bc_module_lcd_draw_string(10, 5, "Waiting for data...", true);

    // Don't forget to update
    bc_module_lcd_update();

    bc_uart_init(BC_UART_UART2, BC_UART_BAUDRATE_921600, BC_UART_SETTING_8N1);
    bc_uart_set_event_handler(BC_UART_UART2, uart_handler, NULL);

    bc_fifo_init(&tx_fifo, tx_fifo_buffer, sizeof(tx_fifo_buffer));
    bc_fifo_init(&rx_fifo, rx_fifo_buffer, sizeof(rx_fifo_buffer));

    bc_uart_set_async_fifo(BC_UART_UART2, &tx_fifo, &rx_fifo);

    bc_uart_async_read_start(BC_UART_UART2, 500);


    // Initialize LCD button left
    static bc_button_t lcd_left;
    bc_button_init_virtual(&lcd_left, BC_MODULE_LCD_BUTTON_LEFT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_left, lcd_button_left_event_handler, NULL);

    // Initialize LCD button right
    static bc_button_t lcd_right;
    bc_button_init_virtual(&lcd_right, BC_MODULE_LCD_BUTTON_RIGHT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_right, lcd_button_right_event_handler, NULL);

    // Initialize red and blue LED on LCD module

    bc_led_init_virtual(&led_lcd_red, BC_MODULE_LCD_LED_RED, bc_module_lcd_get_led_driver(), true);
    bc_led_init_virtual(&led_lcd_green, BC_MODULE_LCD_LED_GREEN, bc_module_lcd_get_led_driver(), true);
    bc_led_init_virtual(&led_lcd_blue, BC_MODULE_LCD_LED_BLUE, bc_module_lcd_get_led_driver(), true);

    // Initialize Accelerometer

    bc_dice_init(&dice, BC_DICE_FACE_UNKNOWN);
    bc_lis2dh12_init(&lis2dh12, BC_I2C_I2C0, 0x19);
    bc_lis2dh12_set_update_interval(&lis2dh12, 500);
    bc_lis2dh12_set_event_handler(&lis2dh12, lis2dh12_event_handler, NULL);

    bc_apds9960_init(&apds9960, BC_I2C_I2C0, 0x39);
    bc_apds9960_set_event_handler(&apds9960, apds9960_event_handler, NULL);
}
/*
void application_task(void)
{
    // Logging in action
    bc_log_debug(".");



       if ( isGestureAvailable() ) {
    switch ( readGesture() ) {
        case DIR_UP:
            bc_log_debug("UP");
            break;
        case DIR_DOWN:
            bc_log_debug("DOWN");
            break;
        case DIR_LEFT:
            bc_log_debug("LEFT");
            break;
        case DIR_RIGHT:
            bc_log_debug("RIGHT");
            break;
        case DIR_NEAR:
            bc_log_debug("NEAR");
            break;
        case DIR_FAR:
            bc_log_debug("FAR");
            break;
        default:
            bc_log_debug("NONE");
        }
    }



    // Plan next run this function after 1000 ms
    bc_scheduler_plan_current_from_now(10);

}
*/
