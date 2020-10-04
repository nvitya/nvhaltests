/*
 * target_rpi4.cpp
 *
 *  Created on: 3 Oct 2020
 *      Author: vitya
 */

#include "board.h"
#include "hwuart.h"
#include "hwdma.h"
#include "string.h"
#include "clockcnt.h"

#include "stdio.h"

#define MAX_TARGET_UARTS      4
#define DMA_BUFFER_SIZE    1024

class TTargetUart
{
public:
	THwUart          uart;
	THwDmaChannel    rxdma;
	THwDmaChannel    txdma;
	THwDmaTransfer   rxfer;
	THwDmaTransfer   txfer;

	uint8_t *        dmabuf_rx = nullptr;
	uint8_t *        dmabuf_tx = nullptr;

	unsigned         dma_read_idx = 0;
	unsigned         dma_write_idx = 0;

	int Open(int adevnum, int atxdma, int atxdmarq, int arxdma, int arxdmarq);
	int Close();
	int Read(void * dstptr, unsigned maxlen);
	int Write(void * srcptr, unsigned len);
};

TTargetUart  target_hwuart_list[MAX_TARGET_UARTS];

int TTargetUart::Open(int adevnum, int atxdma, int atxdmarq, int arxdma, int arxdmarq)
{
	if (!uart.Init(adevnum))
	{
		return -1;
	}

	if (!txdma.Init(atxdma, atxdmarq))
	{
		return -5;
	}

	if (!rxdma.Init(arxdma, arxdmarq))
	{
		return -6;
	}

	uart.DmaAssign(false, &rxdma);
	uart.DmaAssign(true,  &txdma);

	if (!dmabuf_rx)
	{
		dmabuf_rx = hwdma_allocate_dma_buffer(DMA_BUFFER_SIZE);
	}

	if (!dmabuf_tx)
	{
		dmabuf_tx = hwdma_allocate_dma_buffer(DMA_BUFFER_SIZE);
	}

	// start the rx dma

	dma_read_idx = 0;

	rxfer.bytewidth = 1;
	rxfer.count = DMA_BUFFER_SIZE;
	rxfer.dstaddr = dmabuf_rx;  // must be in the address space allocated by hwdma_allocate_dma_buffer
	rxfer.flags = DMATR_CIRCULAR;

	uart.DmaStartRecv(&rxfer);

	return 0;
}

int TTargetUart::Close()
{
	if (uart.devnum < 0)
	{
		return -2; // not opened
	}

	if (rxdma.Active())
	{
		rxdma.Disable();
	}

	if (txdma.Active())
	{
		txdma.Disable();
	}

	uart.devnum = -1;
	return 0;
}

int TTargetUart::Read(void * dstptr, unsigned maxlen)
{
	// transfer the new data from the Circular DMA buffer to the user buffer

	if (maxlen < DMA_BUFFER_SIZE)
	{
		return -1; // dst buffer is too small
	}

	dma_write_idx = DMA_BUFFER_SIZE - rxdma.Remaining();
	if (dma_write_idx >= DMA_BUFFER_SIZE) // should not happen
	{
		dma_write_idx = 0;
	}

	if (dma_write_idx == dma_read_idx)
	{
		return 0; // nothing was arrived so far
	}


	unsigned len = 0;
	if (dma_write_idx > dma_read_idx)  // simple case without wrap-around
	{
		len = dma_write_idx - dma_read_idx;
		memcpy(dstptr, &dmabuf_rx[dma_read_idx], len); // warning: check unaligned speed !!!!
		dma_read_idx = dma_write_idx;
		return len;
	}

	// else dma_write_idx < dma_read_idx, the transfer must be done in two blocks

	len = DMA_BUFFER_SIZE - dma_read_idx;
	if (len > 0)
	{
	  memcpy(dstptr, &dmabuf_rx[dma_read_idx], len); // warning: check unaligned speed !!!!
	}

	if (dma_write_idx > 0)
	{
		uint8_t * dp = (uint8_t *)dstptr;
		dp += len;
		memcpy(dp, &dmabuf_rx[0], dma_write_idx);
		len += dma_write_idx;
	}

	dma_read_idx = dma_write_idx;

	return len;
}

int TTargetUart::Write(void * srcptr, unsigned len)
{
	if (0 == len)
	{
		return 0;
	}

  if (txdma.Active())
  {
  	return 0;  // the previous transfer is still active
  }

  if (len > DMA_BUFFER_SIZE)
  {
  	len = DMA_BUFFER_SIZE;
  }

  // transfer the data to the DMA buffer and start sending
  memcpy(dmabuf_tx, srcptr, len);

	txfer.bytewidth = 1;
	txfer.count = len;
	txfer.srcaddr = dmabuf_tx;  // must be in the address space allocated by hwdma_allocate_dma_buffer
	txfer.flags = 0;

	uart.DmaStartSend(&txfer);

	return len;
}

//-------------------------------------------------

void target_init()
{

}

int target_hwuart_open(int adevid, unsigned abaudrate)  // retuns a handle
{
	// search for already existing

	int i;
	int firstfree = -1;
	TTargetUart * ptuart;
	for (i = 0; i < MAX_TARGET_UARTS; ++i)
	{
		ptuart = &target_hwuart_list[i];
		if (ptuart->uart.devnum == adevid)
		{
			return -2;  // already allocated.
		}

		if ((firstfree < 0) && (ptuart->uart.devnum == -1))
		{
			firstfree = i;
		}
	}

	if (firstfree < 0)
	{
		return -8; // no more space
	}

	ptuart = &target_hwuart_list[firstfree];

	ptuart->uart.baudrate = abaudrate;

	if (4 == adevid) // uart4
	{
		//DMA RQ:
		// 30=UART4-TX
		// 31=UART4-RX
		return ptuart->Open(adevid, 3, 30, 2, 31);
	}
	else if (5 == adevid) // uart5
	{
		//DMA RQ:
		// 21=UART5-TX
		// 22=UART5-RX
		return ptuart->Open(adevid, 5, 21, 4, 22);
	}
	else
	{
		return -3; // unknown uart
	}

	return 0;
}

int target_hwuart_close(int handle)
{
	if ((handle < 0) || (handle >= MAX_TARGET_UARTS))
	{
		return -1;  // invalid handle
	}

	TTargetUart * ptuart = &target_hwuart_list[handle];
	return ptuart->Close();
}

int target_hwuart_read(int handle, void * dstptr, unsigned maxlen)
{
	if ((handle < 0) || (handle >= MAX_TARGET_UARTS))
	{
		return -1;  // invalid handle
	}

	TTargetUart * ptuart = &target_hwuart_list[handle];

	return ptuart->Read(dstptr, maxlen);
}

int target_hwuart_write(int handle, void * srcptr, unsigned len)
{
	if ((handle < 0) || (handle >= MAX_TARGET_UARTS))
	{
		return -1;  // invalid handle
	}

	TTargetUart * ptuart = &target_hwuart_list[handle];

	return ptuart->Write(srcptr, len);
}
