/*
 *  file:     uartdma_main.cpp
 *  brief:    NVHAL Uart DMA Tests
 *  version:  1.00
 *  date:     2020-09-29
 *  authors:  nvitya
*/

#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "string.h"

#include "clockcnt.h"
#include "hwpins.h"
#include "hwuart.h"
#include "hwdma.h"

#include "hw_utils.h"
#include "broadcom_utils.h"

#include "target_rpi4.h"

int uart_handle = -1;

uint8_t txbuf[1024]  __attribute__((aligned(32)));
uint8_t rxbuf[2048]  __attribute__((aligned(32)));  // minimum 1k required

bool wait_uart_response(int expectedlen, int timeout_us)
{
	clockcnt_t st = CLOCKCNT;
	int rxlen = 0;
	int r;

	while (rxlen < expectedlen)
	{
		r = target_hwuart_read(uart_handle, &rxbuf[rxlen], 1024);
		if (r > 0)
		{
			rxlen += r;
		}
		else if (int(CLOCKCNT - st) > timeout_us)
		{
			printf("receive timeout");
			return false;
		}
	}

	if (rxlen > expectedlen)
	{
		printf("More bytes are received than expected (%i)\n", rxlen);
		return false;
	}

	return true;
}

int main()
{
	printf("RPI4 UART test\n");

	clockcnt_init();

	printf("Opening uart5 with 3 MBit/s ...\n");
	uart_handle = target_hwuart_open(5, 3000000);  // open uart5 with 3 MBit/s
	if (uart_handle < 0)
	{
		printf("Error opening uart\n");
		exit(1);
	}

	printf("uart opened.\n");

	// assembly reg read the request 1
	txbuf[0] = 0x55;
	txbuf[1] = 0xF2;
	txbuf[2] = 0x00;
	txbuf[3] = 0x00;
	txbuf[4] = (0xFF ^ txbuf[1] ^ txbuf[2] ^ txbuf[3]);  // checksum

	// assembly reg read the request 2
	txbuf[8]  = 0x55;
	txbuf[9]  = 0xF2;
	txbuf[10] = 0x01;
	txbuf[11] = 0x00;
	txbuf[12] = (0xFF ^ txbuf[9] ^ txbuf[10] ^ txbuf[11]); // checksum

	int r;
	clockcnt_t t0, t1;

	int rspcnt = 0;

	uint32_t rspvalue;

	t0 = 0;

	printf("Repeating requests...\n");

	// empty receive buffer
	r = target_hwuart_read(uart_handle, &rxbuf[0], 1024);

	while (1)
	{
		t1 = CLOCKCNT;

		// send request 1
		r = target_hwuart_write(uart_handle, &txbuf[0], 5);
		if (r != 5)
		{
			printf("rq 1 send error: %i!\n", r);
			exit(1);
		}

#if 0
		// let the DMA collect all bytes
		delay_us(100);

		// read out response 1
		r = target_hwuart_read(uart_handle, &rxbuf[0], 1024);
		if (r != 9)
		{
			printf("rsp 1 length error: %i\n", r);
			exit(1);
		}
#else
		if (!wait_uart_response(9, 200))
		{
			exit(1); // error reason already printed.
		}
#endif

		// evaluate the response 1
		rspvalue = *(uint32_t *)&rxbuf[4];
		if (rspvalue != 0x87654321)
		{
			printf("rsp 1 value error: %08X\n", rspvalue);
			exit(1);
		}

		++rspcnt;	// ok.

		// send request 2, which is different from rq1
		r = target_hwuart_write(uart_handle, &txbuf[8], 5);
		if (r != 5)
		{
			printf("rq 2 send error: %i!\n", r);
			exit(1);
		}

#if 0
		// let the DMA collect all bytes
		delay_us(100);

		// read out response 2
		r = target_hwuart_read(uart_handle, &rxbuf[0], 1024);
		if (r != 9)
		{
			printf("rsp 2 length error: %i\n", r);
			exit(1);
		}
#else
		if (!wait_uart_response(9, 200))
		{
			exit(1); // error reason already printed.
		}
#endif

		// evaluate the response 2
		rspvalue = *(uint32_t *)&rxbuf[4];
		if (rspvalue != 0x00005ECA)
		{
			printf("rsp 2 value error: %08X\n", rspvalue);
			exit(1);
		}

		++rspcnt; // ok.

		if (t1 - t0 > CLOCKCNT_SPEED)
		{
		  printf("rspcnt = %u\n", rspcnt);
		  t0 = t1;
		}
	}

	return 0;
}
