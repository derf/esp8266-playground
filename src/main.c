#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "gpio.h"
#include "mem.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

os_event_t    user_procTaskQueue[user_procTaskQueueLen];

LOCAL os_timer_t hello_timer;
LOCAL scaninfo *pscaninfo;

extern u16 scannum;

static void ICACHE_FLASH_ATTR scan_cb(void *arg, STATUS status)
{
	WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(0));

	if (pscaninfo == NULL) {
		pscaninfo = (scaninfo *)os_zalloc(sizeof(scaninfo));
	}

	pscaninfo->pbss = arg;
	if (scannum % 8 == 0)
		pscaninfo->totalpage = scannum / 8;
	else
		pscaninfo->totalpage = scannum / 8 + 1;

	os_printf("pagenum %d/%d", pscaninfo->pagenum, pscaninfo->totalpage);
}

//Hello world task
static void ICACHE_FLASH_ATTR
task_hello(os_event_t *events)
{
	struct scan_config sc;
	sc.ssid = NULL;
	sc.bssid = NULL;
	sc.channel = 0;
	sc.show_hidden = 1;

	WRITE_PERI_REG(PAD_XPD_DCDC_CONF, (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1);
	WRITE_PERI_REG(RTC_GPIO_CONF, (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);
	WRITE_PERI_REG(RTC_GPIO_ENABLE, (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);

	wifi_station_set_hostname( "noderf" );
	wifi_set_opmode_current( STATION_MODE );
	wifi_station_scan(&sc, scan_cb);
	WRITE_PERI_REG(RTC_GPIO_OUT, (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(1));

	//print text to the serial output
	os_printf("Hello World!\r\n");
}

static void ICACHE_FLASH_ATTR timer_hello(void *arg)
{
	static int wat = 0;
	os_printf("Hello Timer!\r\n");
	os_printf("gpio %x\r\n", gpio_input_get());

	if (wat) {
		wat = 0;
		gpio_output_set(0, BIT2, BIT2, 0);
	} else {
		wat = 1;
		gpio_output_set(BIT2, 0, BIT2, 0);
	}
}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
  // Initialize UART0
  uart_div_modify(0, UART_CLK_FREQ / 115200);

  gpio_init();
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);

  os_timer_disarm(&hello_timer);
  os_timer_setfn(&hello_timer, (os_timer_func_t *)timer_hello, (void *)0);
  os_timer_arm(&hello_timer, 1000, 1);

  //Set up the hello world task
  system_os_task(task_hello, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
  //send a message to the "hello world" task to activate it
  system_os_post(user_procTaskPrio, 0, 0 );
}