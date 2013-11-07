#include <stdio.h>
#include <unistd.h>       /* for sleep() */
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/neutrino.h> /* for ThreadCtl() */
#include <sys/mman.h>     /* for mmap_device_io() */
#include <pthread.h>
#include <time.h>
#include <sys/netmgr.h>


/* The Neutrino IO port used here corresponds to a single register, which is
 * one byte long */
#define PORT_LENGTH 1

/*Digital input output port configuration,*/
#define DIO_BASE_ADDR 0x280
#define DIO_PORTC_ADDR 0x0A
#define DIO_PORTB_ADDR 0x09
#define DIO_CTL_ADDR 0x0B
#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

 /* bit 2 = printer initialisation (high to initialise)
  * bit 4 = hardware IRQ (high to enable) */
#define ENABLE_IN 0xFF
#define ENABLE_OUT 0x00

#define LOW 0x00
#define HIGH 0x01

int user_input = 1;
uintptr_t ctrl_handle_portB;
struct timespec my_timer_value1;
struct timespec my_timer_value2;

void *pulse_to_sensor( void *ptr );

typedef union {
        struct _pulse   pulse;
} my_message_t;  //This union is for timer module.

/* ______________________________________________________________________ */
int
main( )
{
    int privity_err, timerThread;
    long unsigned start_time = 0, end_time = 0;
    uintptr_t ctrl_handle_portA;
    uintptr_t ctrl_handle_portCTL;

    pthread_t thread0;

    //my_timer_value1.tv_nsec = 1000;
    my_timer_value1.tv_nsec = 0;
    my_timer_value1.tv_sec = 1000;

    my_timer_value2.tv_nsec = 0;
    my_timer_value2.tv_sec = 0.99999;

    timerThread = pthread_create( &thread0, NULL, pulse_to_sensor, (void*) NULL);

    /* Give this thread root permissions to access the hardware */
    privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
    if ( privity_err == -1 )
    {
        fprintf( stderr, "can't get root permissions\n" );
        return -1;
    }

    /* Get a handle to the DIO port's Control register */
    ctrl_handle_portA = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + 0x08 );
    ctrl_handle_portB = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_PORTB_ADDR);
    ctrl_handle_portCTL = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_CTL_ADDR);

    /* Initialize the DIO port */
    out8( ctrl_handle_portCTL, 0x10 );
   // loop until there is a key press to stop
    while(1){
 /*   	out8( ctrl_handle_portB, HIGH );
    	nanospin( &my_timer_value1 );
    	out8( ctrl_handle_portB, LOW );
    	nanospin( &my_timer_value2 );*/
    	if (in8(ctrl_handle_portA) & 0x01)
    	{
       		start_time = clock();
       		//printf("Sensor is sending something.\n");
    	}
    	while(in8(ctrl_handle_portA) & 0x01){
    		//printf("doing nothing\n");
    	}
    	end_time = clock();
    	printf("end_Time: %lu and start_time: %lu and diff: %lu \n", end_time, start_time, ((end_time-start_time)/CLOCKS_PER_SEC));
    }
    pthread_join( thread0, NULL);
    return 0;
}


void *pulse_to_sensor( void *ptr )
{
	int privity_err;
	  printf("Pulse_Thread\n");
	  privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	  if ( privity_err == -1 )
	  {
	      fprintf( stderr, "can't get root permissions\n" );
	      pthread_exit(NULL);
	  }
	  while(user_input){
		  out8( ctrl_handle_portB, HIGH );
		  nanospin( &my_timer_value1 );
		  out8( ctrl_handle_portB, LOW );
		  nanospin( &my_timer_value2 );
	  }

	  pthread_exit(NULL);
}
