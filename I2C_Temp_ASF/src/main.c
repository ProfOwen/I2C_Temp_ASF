
#include <asf.h>

/************************************************************************/
/* Global Variables                                                     */
/************************************************************************/

struct usart_module usart_instance;
#define MAX_RX_BUFFER_LENGTH   5
volatile uint8_t rx_buffer[MAX_RX_BUFFER_LENGTH];
volatile unsigned int sys_timer1 = 0;

double temp_res;

/************************************************************************/
/* Function Prototypes                                                  */
/************************************************************************/

void usart_read_callback(const struct usart_module *const usart_module);
void usart_write_callback(const struct usart_module *const usart_module);

/************************************************************************/
/* INIT Clocks                                                          */
/************************************************************************/

void enable_tc_clocks(void)
{
	
	struct system_gclk_chan_config gclk_chan_conf;
	
	/* Turn on TC module in PM */
	system_apb_clock_set_mask(SYSTEM_CLOCK_APB_APBC, PM_APBCMASK_TC3);

	/* Set up the GCLK for the module */
	system_gclk_chan_get_config_defaults(&gclk_chan_conf);
	
	//Setup generic clock 0 (also the clock for MCU (running at 8 Mhz) as source for the timer clock)
	gclk_chan_conf.source_generator = GCLK_GENERATOR_0;
	system_gclk_chan_set_config(TC3_GCLK_ID, &gclk_chan_conf);
	
	//Enable the generic clock for the Timer/ Counter block
	system_gclk_chan_enable(TC3_GCLK_ID);
}

/************************************************************************/
/* INIT USART                                                           */
/************************************************************************/

void configure_usart(void)
{
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);
	config_usart.baudrate    = 115200;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	while (usart_init(&usart_instance,
	EDBG_CDC_MODULE, &config_usart) != STATUS_OK) {
	}
	usart_enable(&usart_instance);
}

void configure_usart_callbacks(void)
{
	usart_register_callback(&usart_instance,
	usart_write_callback, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_register_callback(&usart_instance,
	usart_read_callback, USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);
}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/

int main (void)
{
	system_init();
	system_clock_init();
	at30tse_init();
	
	enable_tc_clocks();
	SysTick_Config(8000);
	
	configure_usart();
	configure_usart_callbacks();
	
	// Write a confirmation string to PC
	uint8_t string[] = "This is the SAMD20 with your Temp readings!\r\n";
	usart_write_buffer_job(&usart_instance, string, sizeof(string));
	
	
	// Sometimes when I use ASF i feel like I am writing a novel.
	volatile uint16_t thigh = 0;
	thigh = at30tse_read_register(AT30TSE_THIGH_REG,
	AT30TSE_NON_VOLATILE_REG, AT30TSE_THIGH_REG_SIZE);
	volatile uint16_t tlow = 0;
	tlow = at30tse_read_register(AT30TSE_TLOW_REG,
	AT30TSE_NON_VOLATILE_REG, AT30TSE_TLOW_REG_SIZE);
	/* Set 12-bit resolution mode. */
	at30tse_write_config_register(
	AT30TSE_CONFIG_RES(AT30TSE_CONFIG_RES_12_bit));
	while (1) 
	{
		if (sys_timer1 > 1000)
		{
			temp_res = at30tse_read_temperature();
			
			if (temp_res < 0.0)
			{
				temp_res = ((128 + temp_res)*-1);	
			}
			
			char tx_buffer[100];
			sprintf(tx_buffer, "Temperature = %f \r\n", temp_res);
			usart_write_buffer_job(&usart_instance, tx_buffer, sizeof(tx_buffer));
			
			// Reset sys_timer1
			sys_timer1 = 0;
		}
		
	}
	
}

/************************************************************************/
/* Subroutines                                                          */
/************************************************************************/

void usart_read_callback(const struct usart_module *const usart_module)
{
	usart_write_buffer_job(&usart_instance,
	(uint8_t *)rx_buffer, MAX_RX_BUFFER_LENGTH);
}

void usart_write_callback(const struct usart_module *const usart_module)
{
	port_pin_toggle_output_level(LED_0_PIN);
}

void SysTick_Handler(void)
{
	sys_timer1++;
}
