/*
 *  file:     uarttest.cpp (NVHAL uart test)
 *  brief:    NVHAL Uart Tests
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

THwUart uart;

THwDmaChannel  dmarx;
THwDmaChannel  dmatx;

THwDmaTransfer rxfer;
THwDmaTransfer txfer;

uint8_t normal_mem[4096]  __attribute__((aligned(32)));
uint8_t normal_mem2[4096]  __attribute__((aligned(32)));

volatile uint64_t g_dummy = 0;

void mem_speed_test64(uint8_t * mem, unsigned size, unsigned repeat)
{
	clockcnt_t t0, t1;

	uint64_t * endp = (uint64_t *)mem;
	endp += (size >> 3);

	t0 = CLOCKCNT;
	for (unsigned n = 0; n < repeat; ++n)
	{
		uint64_t * dp = (uint64_t *)mem;
		while (dp < endp)
		{
			*dp++ = n;
		}
	}
	t1 = CLOCKCNT;
	printf("write speed64 test clocks: %i\n", int(t1-t0));

	uint32_t sum = 0;
	t0 = CLOCKCNT;
	for (unsigned n = 0; n < repeat; ++n)
	{
		uint64_t * dp = (uint64_t *)mem;
		while (dp < endp)
		{
			sum += *dp++;
		}
	}
	t1 = CLOCKCNT;
	g_dummy += sum; // to cheat the optimizer
	printf("read speed64 test clocks: %i\n", int(t1-t0));

	t0 = CLOCKCNT;
	for (unsigned n = 0; n < repeat; ++n)
	{
		memcpy(normal_mem2, mem, size);
	}
	t1 = CLOCKCNT;
	printf("memcpy read test clocks: %i\n", int(t1-t0));

	t0 = CLOCKCNT;
	for (unsigned n = 0; n < repeat; ++n)
	{
		memcpy(mem, normal_mem2, size);
	}
	t1 = CLOCKCNT;
  printf("memcpy write test clocks: %i\n", int(t1-t0));

}

void mem_speed_test32(uint8_t * mem, unsigned size, unsigned repeat)
{
	clockcnt_t t0, t1;

	uint32_t * endp = (uint32_t *)mem;
	endp += (size >> 2);

	t0 = CLOCKCNT;

	for (unsigned n = 0; n < repeat; ++n)
	{
		uint32_t * dp = (uint32_t *)mem;
		while (dp < endp)
		{
			*dp++ = n;
		}
	}

	t1 = CLOCKCNT;

	printf("write speed32 test clocks: %i\n", int(t1-t0));

	uint32_t sum = 0;

	t0 = CLOCKCNT;

	for (unsigned n = 0; n < repeat; ++n)
	{
		uint32_t * dp = (uint32_t *)mem;
		while (dp < endp)
		{
			sum += *dp++;
		}
	}

	t1 = CLOCKCNT;

	g_dummy += sum; // to cheat the optimizer

	printf("read speed32 test clocks: %i\n", int(t1-t0));

}


void mem_speed_test8(uint8_t * mem, unsigned size, unsigned repeat)
{
	clockcnt_t t0, t1;

	uint8_t * endp = mem + size;

	t0 = CLOCKCNT;

	for (unsigned n = 0; n < repeat; ++n)
	{
		uint8_t * dp = mem;
		while (dp < endp)
		{
			*dp++ = n;
		}
	}

	t1 = CLOCKCNT;

	printf("speed8 test clocks: %i\n", int(t1-t0));
}

uint8_t * uart_txbuf = nullptr;
uint8_t * uart_rxbuf = nullptr;

const char * uart_teststring = "[This is a very long uart test string that does not fit into the UART FIFO and will be sent using DMA.]";

void uart_dma_tx_test()
{
	printf("UART DMA TX Test...\n");

	//dmarx.Init(4, 22);

	//dmatx.Init(5, 30);  // 30=UART4-TX
	dmatx.Init(5, 21);  // 21=UART5-TX

	//dmatx.Init(5, 0);

	//uart.DmaAssign(false, &dmarx);
	uart.DmaAssign(true,  &dmatx);

	uart_txbuf = hwdma_allocate_dma_buffer(1024);
	//uart_rxbuf = hwdma_allocate_dma_buffer(1024);

#if 0
	// fill the tx buffer with some data (32 bit / char yet)

	uart_txbuf[0] = '0';
	uart_txbuf[4] = '1';
	uart_txbuf[8] = '2';
	uart_txbuf[12] = '3';

	txfer.bytewidth = 4;
	txfer.count = 4;
	txfer.srcaddr = &uart_txbuf[0];  // must be in the address space allocated by hwdma_allocate_dma_buffer
	txfer.flags = 0;

	uart.DmaStartSend(&txfer);

	delay_us(1000000);

	printf("4 characters are sent via DMA, exiting.\n");
#else
	// try bytewise
	strcpy((char *)uart_txbuf, uart_teststring);

	txfer.bytewidth = 1;
	txfer.count = strlen(uart_teststring);
	txfer.srcaddr = &uart_txbuf[0];  // must be in the address space allocated by hwdma_allocate_dma_buffer
	txfer.flags = 0;

	printf("sending %i characters via DMA...\n", txfer.count);
	uart.DmaStartSend(&txfer);
	delay_us(1000000);

	printf("finished, exiting.\n");

#endif

	exit(1);
}

void uart_dma_rx_test()
{
	printf("UART DMA RX Test...\n");


	//uart_txbuf = hwdma_allocate_dma_buffer(1024);
	uart_rxbuf = hwdma_allocate_dma_buffer(1024);

	unsigned rx_buffer_size = 4;

	rxfer.bytewidth = 1;
	rxfer.count = rx_buffer_size;
	rxfer.dstaddr = &uart_rxbuf[0];  // must be in the address space allocated by hwdma_allocate_dma_buffer
	rxfer.flags = DMATR_CIRCULAR;

	uart.DmaStartRecv(&rxfer);

	printf("Waiting for rx characters...\n");


	for (int n=0; n < 12; ++n)  uart_rxbuf[n] = '-';

#if 1
	uint8_t c;
	unsigned dma_read_idx = 0;

	while (true)
	{
		unsigned dma_write_idx = rx_buffer_size - dmarx.Remaining();
		if (dma_write_idx >= rx_buffer_size) // should not happen
		{
			dma_write_idx = 0;
		}

		while (dma_write_idx != dma_read_idx)
		{
			c = uart_rxbuf[dma_read_idx];

			printf("RX(%i): %c  ", dma_read_idx, c);

			for (int n=0; n < 12; ++n)  printf("%c", uart_rxbuf[n]);
			printf("\n");

			++dma_read_idx;
			if (dma_read_idx >= rx_buffer_size)  dma_read_idx = 0;
		}

		if (!dmarx.Active())  break;
	}
#else
	unsigned prev_remaining = 0xFFFFF;

	while (true)
	{
		unsigned new_remaining = dmarx.Remaining();
		if ((prev_remaining != new_remaining) || !dmarx.Active())
		{
			//printf("rx remaining=%i: ", new_remaining);
			printf("TXFR_LEN=%08X  ", dmarx.regs->TXFR_LEN);

			for (int n=0; n < 12; ++n)  printf("%c", uart_rxbuf[n]);
			printf("\n");

			prev_remaining = new_remaining;

			if (!dmarx.Active())  break;
		}
	}

#endif


	printf("finished, exiting.\n");

	exit(1);
}


void uncached_memory_test()
{
	printf("Testing uncached memory...\n");

	unsigned mem_handle = broadcom_vpu_mem_alloc(4096);
	printf(" mem_handle = %08X\n", mem_handle);

	unsigned bus_addr = broadcom_vpu_mem_lock(mem_handle);
	printf(" bus_addr   = %08X\n", bus_addr);

	unsigned phys_addr = (bus_addr & 0x3FFFFFFF);
	printf(" phys_addr  = %08X\n", phys_addr);

	uint8_t * uncached_mem_ptr = (uint8_t *)hw_memmap(phys_addr, 4096);

	printf("testing normal mem speed...\n");
	mem_speed_test8(&normal_mem[0], 4096, 1000);
	mem_speed_test32(&normal_mem[0], 4096, 1000);
	mem_speed_test64(&normal_mem[0], 4096, 1000);

	printf("testing uncached mem speed...\n");
	mem_speed_test8(uncached_mem_ptr, 4096, 1000);
	mem_speed_test32(uncached_mem_ptr, 4096, 1000);
	mem_speed_test64(uncached_mem_ptr, 4096, 1000);

	printf("test finished.\n");
	exit(1);
}

int main()
{
	printf("RPI4 UART test\n");

	clockcnt_init();

	//uncached_memory_test();

	if (!uart.Init(5))
	{
		printf("Error initializing HwUart(%i)!\n", uart.devnum);
		exit(1);
	}

	dmatx.Init(5, 21);  // 21=UART5-TX
	dmarx.Init(4, 22);  // 22=UART5-RX

	uart.DmaAssign(false, &dmarx);
	uart.DmaAssign(true,  &dmatx);


	//uart_dma_tx_test();
	uart_dma_rx_test();



	char c;
	clockcnt_t t0, t1;

	t0 = 0;

	printf("Sending UART Char...\n");

	unsigned cnt = 0;
	while (1)
	{
		t1 = CLOCKCNT;

		if (uart.TryRecvChar(&c))
		{
			printf("received: %c\n", c);
		}

		if (t1 - t0 > CLOCKCNT_SPEED)
		{
		  ++cnt;
		  printf("cnt = %u\n", cnt);

		  uart.SendChar(0x55);
		  uart.SendChar(0x55);
		  //uart.SendChar(0x00);
		  //uart.SendChar(0x00);
		  t0 = t1;
		}
	}

	return 0;
}
