/*
 *  file:     uarttest.cpp (NVHAL uart test)
 *  brief:    NVHAL CLOCKCNT / GPIO Tests
 *  version:  1.00
 *  date:     2020-09-29
 *  authors:  nvitya
*/

#include "stdio.h"
#include "time.h"

#include "clockcnt.h"
#include "hwpins.h"

TGpioPin pin_out(0, 20, false);
TGpioPin pin_in(0, 21, false);

uint64_t nstime_sys(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * (uint64_t)1000000000 + (uint64_t)ts.tv_nsec;
}

int main()
{
	printf("RPI4 GPIO test\n");

	clockcnt_init();
	pin_out.Setup(PINCFG_OUTPUT | PINCFG_GPIO_INIT_1);
	pin_in.Setup(PINCFG_INPUT | PINCFG_PULLUP);

	clockcnt_t t[4];

	t[0] = CLOCKCNT;
	t[1] = CLOCKCNT;
	t[2] = CLOCKCNT;
	t[3] = CLOCKCNT;

	printf("CLOCKCNT consecutive calls:\n");
	printf(" %i\n", int(t[1] - t[0]));
	printf(" %i\n", int(t[2] - t[1]));
	printf(" %i\n", int(t[3] - t[2]));

	// calibrate the system clock
	t[0] = CLOCKCNT;
	uint64_t st0;
	st0 = nstime_sys();
	while (nstime_sys() - st0 < 1000000000)
	{
		// wait
	}
	t[1] = CLOCKCNT;

	printf("1s clockcnt = %u\n", unsigned(t[1] - t[0]));

	printf("actual counter %llu\n", CLOCKCNT);

	printf("Toggling GPIO...\n");

	unsigned cnt = 0;
	while (1)
	{
		printf("cnt = %u\n", cnt);
		pin_out.SetTo(cnt & 1);
		delay_ms(1000);
		printf(" input value = %i\n", pin_in.Value());
		++cnt;
	}

	return 0;
}
