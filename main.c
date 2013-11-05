#include <stdio.h>
#include <unistd.h>       /* for sleep() */
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/neutrino.h> /* for ThreadCtl() */
#include <sys/mman.h>     /* for mmap_device_io() */

/* The Neutrino IO port used here corresponds to a single register, which is
 * one byte long */
#define PORT_LENGTH 1

/*Digital input output port configuration,*/
#define DIO_BASE_ADDR 0x280
#define DIO_PORTC_ADDR 0x0A
#define DIO_PORTB_ADDR 0x09
#define DIO_CTL_ADDR 0x0B

 /* bit 2 = printer initialisation (high to initialise)
  * bit 4 = hardware IRQ (high to enable) */
#define ENABLE_IN 0xFF
#define ENABLE_OUT 0x00

#define LOW 0x00
#define HIGH 0xFF

/* ______________________________________________________________________ */
int
main( )
{
	int privity_err;
	uintptr_t ctrl_handle_portC;
	uintptr_t ctrl_handle_portB;
	uintptr_t ctrl_handle_portCTL;

    struct timespec my_timer_value;
    my_timer_value.tv_nsec = 100000000;


	/* Give this thread root permissions to access the hardware */
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
		return -1;
	}

	/* Get a handle to the DIO port's Control register */
	//ctrl_handle_portA = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + 0x08 );
	ctrl_handle_portB = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_PORTB_ADDR);
	ctrl_handle_portCTL = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_CTL_ADDR);

	/* Initialise the DIO port */
	out8( ctrl_handle_portCTL, 0x00 );
	//out8( ctrl_handle_portA, ENABLE_IN );
	//out8( ctrl_handle_portB, ENABLE_OUT );


	for (;;)
	{
		//out8( ctrl_handle_portB, HIGH );
		out8( ctrl_handle_portB, HIGH );
		nanospin( &my_timer_value );
		//out8( ctrl_handle_portB, LOW );
		out8( ctrl_handle_portB, LOW );
		nanospin( &my_timer_value );
	}

	return 0;
}
