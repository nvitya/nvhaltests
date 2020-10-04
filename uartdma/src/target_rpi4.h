/*
 * target_rpi4.h
 *
 *  Created on: 3 Oct 2020
 *      Author: vitya
 */

#ifndef TARGET_RPI4_TARGET_RPI4_H_
#define TARGET_RPI4_TARGET_RPI4_H_

extern void target_init();

// UART functions using DMA:

extern int target_hwuart_open(int adevid, unsigned abaudrate);  // retuns a handle
extern int target_hwuart_close(int handle);
extern int target_hwuart_read(int handle, void * dstptr, unsigned maxlen);
extern int target_hwuart_write(int handle, void * srcptr, unsigned len);

#endif /* TARGET_RPI4_TARGET_RPI4_H_ */
