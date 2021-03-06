#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>       /* for sleep() */
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/neutrino.h> /* for ThreadCtl() */
#include <sys/mman.h>     /* for mmap_device_io() */
#include <pthread.h>
#include <time.h>
#include <sys/netmgr.h>
#include <math.h>


/* The Neutrino IO port used here corresponds to a single register, which is
 * one byte long */
#define PORT_LENGTH 1

/*Digital input output port configuration,*/
#define DIO_BASE_ADDR 0x280                //Base Address for Ports
#define DIO_PORTA_ADDR 0x08				   //Base Address for Port_A
#define DIO_PORTB_ADDR 0x09				   //Base Address for Port_B
#define DIO_CTL_ADDR 0x0B
#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

 /* bit 2 = printer initialisation (high to initialise)
  * bit 4 = hardware IRQ (high to enable) */
#define ENABLE_IN 0xFF
#define ENABLE_OUT 0x00

#define LOW 0x00  					//Values to give pulse.
#define HIGH 0x01
#define SPEED_OF_LIGHT 11784.96    // speed of light in Inch per Micro seconds

int user_input = 0;
int start_time = 0;
int end_time = 0;
int diff =0;
int first_reading = 0;

double max_distance = 0, min_distance = 0;      //this will save the max and min distance
double distance;

unsigned long timer_count =0;                   //Variable to take care the time.

/* Handlers for port-A and port-B */
uintptr_t ctrl_handle_portA;
uintptr_t ctrl_handle_portB;

/*Structures for timers used.*/
struct timespec my_timer_value1;
struct timespec my_timer_value2;

/*Threads functions.*/
void *pulse_to_sensor( void *ptr );
void *timer( void *ptr );
void *sensor_output(void *ptr);

typedef union {
        struct _pulse   pulse;
} my_message_t;  //This union is for timer module.

int main( )
{
	fflush(stdout);
    int privity_err, timerThread,timerThread1, timerThread2;

    uintptr_t ctrl_handle_portCTL;

    pthread_t thread0, thread1, thread2;

    //my_timer_value1.tv_nsec = 1000;
    my_timer_value1.tv_nsec = 10000;
    my_timer_value1.tv_sec = 0;
    my_timer_value2.tv_nsec = 10000000;
    my_timer_value2.tv_sec = 0;

    /* Give this thread root permissions to access the hardware */
    privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
    if ( privity_err == -1 )
    {
        fprintf( stderr, "can't get root permissions\n" );
        return -1;
    }

    /* Get a handle to the DIO port's Control register */
    ctrl_handle_portA = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_PORTA_ADDR );
    ctrl_handle_portB = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_PORTB_ADDR);
    ctrl_handle_portCTL = mmap_device_io( PORT_LENGTH, DIO_BASE_ADDR + DIO_CTL_ADDR);

    /* Initialize the DIO port */
    out8( ctrl_handle_portCTL, 0x10 );

    char c;
    printf("\nHey, \n s : Start the sensor and e : stop the sensor.\n Choose your option: ");
    while((c = getchar()) != EOF)
    {
    	if(c != '\n')
    	{
    		if ( c == 's' ){
    		first_reading = 1;
    		user_input = 1;
    		printf("It's s;\n");
    		system("clear");
    		fflush(stdout);
    	    timerThread = pthread_create( &thread0, NULL, pulse_to_sensor, (void*) NULL);
    	    timerThread1 = pthread_create( &thread1, NULL, timer, (void*) NULL);
    	    timerThread2 = pthread_create( &thread2, NULL, sensor_output, (void*) NULL);

    	}
    	else if(c == 'e')
    	{
    		printf("It's e;\n");
    		user_input = 0;
    		fflush(stdout);
    		printf("Max distance : %f, Min Distance : %f", max_distance, min_distance);
    		pthread_join( thread0, NULL);
    		pthread_join( thread1, NULL);
    		pthread_join( thread2, NULL);

    	}
    	else{
    		printf("\nThat was wrong entry, please try again:\n");
    		printf("\ns : Start the sensor and e : stop the sensor.\n Choose your option: ");
    	}
    		printf("\nHey, \n s : Start the sensor and e : stop the sensor.\n Choose your option: ");
    	}
    	fflush(stdout);
    }
    return 0;
}

void *sensor_output( void *ptr )
{
	//printf("sensor_output thread.\n");
	int privity_err;
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
	    pthread_exit(NULL);
	}
	int detect_flag = 0;
	while(user_input){
		if ((in8(ctrl_handle_portA) & 0x01) && detect_flag == 0)
	    {
			start_time = timer_count;
	        detect_flag = 1;
	        //printf("Sensor is sending something.\n");
	    }
	    while((in8(ctrl_handle_portA) & 0x01) && (user_input == 1)){
	    	//doing nothing.
	        detect_flag = 0;
	    }
	    if(detect_flag == 0){
	    	end_time = timer_count;
	        diff = end_time - start_time;
	        distance = (double)(SPEED_OF_LIGHT * diff/20000);
	        if (first_reading)
	        {
	        	min_distance = distance;
	        	max_distance = distance;
	        	first_reading = 0;
	        }
	        if(distance < min_distance)
	        	min_distance = distance;
	        if(distance > max_distance)
                max_distance = distance;
            //printf("timer: %u\n", timer_count);
	    }
	    if(timer_count % 10 == 0){
	    	//printf("I am here.\n");
	        if (diff<18000000 && diff>100)/* && (detect_flag == 0))*/
	        {
	        	printf("\r%f inches",distance );
	            //system("clear");
	            //printf("\r");
	        }
	        else{
	             printf("\r############################");
	        }
	     }
	}
	//printf("sensor_output: I am done.\n");
	pthread_exit(NULL);
}
void *timer( void *ptr )
{
   //printf("Timer Thread.\n");
   timer_count = 0;
   struct sigevent         event;
   struct itimerspec       itime;
   timer_t                 timer_id;
   int                     chid;
   int                     rcvid;
   my_message_t            msg;

   chid = ChannelCreate(0);

   event.sigev_notify = SIGEV_PULSE;
   event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
                                    chid,
                                    _NTO_SIDE_CHANNEL, 0);
   event.sigev_priority = getprio(0);
   event.sigev_code = MY_PULSE_CODE;
   timer_create(CLOCK_REALTIME, &event, &timer_id);

   itime.it_value.tv_sec = 0;
   itime.it_value.tv_nsec = 10000;
   itime.it_interval.tv_sec = 0;
   itime.it_interval.tv_nsec = 10000;

   timer_settime(timer_id, 0, &itime, NULL);

   /*
    * As of the timer_settime(), we will receive our pulse
    * in 1u seconds (the itime.it_value) and every 1u
    * seconds thereafter (the itime.it_interval)
    */
   while(user_input){
	   rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
	   timer_count++;
   }
   //printf("\ntimer : Exiting\n");    //Debug log
   pthread_exit(NULL);
}


void *pulse_to_sensor( void *ptr )
{
        int privity_err;
        //printf("Pulse_Thread\n");
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
        //printf("pulse_to_sensor: Exiting\n");    //Debug Log
        pthread_exit(NULL);
}
