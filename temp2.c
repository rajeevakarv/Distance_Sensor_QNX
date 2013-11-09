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
#define DIO_BASE_ADDR 0x280
#define DIO_PORTA_ADDR 0x08
#define DIO_PORTB_ADDR 0x09
#define DIO_CTL_ADDR 0x0B
#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL

 /* bit 2 = printer initialisation (high to initialise)
  * bit 4 = hardware IRQ (high to enable) */
#define ENABLE_IN 0xFF
#define ENABLE_OUT 0x00

#define LOW 0x00
#define HIGH 0x01
#define SPEED_OF_LIGHT 11784.96    // speed of light in Inch per Micro seconds

int user_input = 1;
int start_time = 0;
int end_time = 0;
int diff =0;
static int timer_count;

uintptr_t ctrl_handle_portB;
struct timespec my_timer_value1;
struct timespec my_timer_value2;

void *pulse_to_sensor( void *ptr );
void *timer( void *ptr );
typedef union {
        struct _pulse   pulse;
} my_message_t;  //This union is for timer module.

/* ______________________________________________________________________ */
int main( )
{
    int privity_err, timerThread,timerThread1;
    int max_distance = 0, min_distance = 0;

    uintptr_t ctrl_handle_portA;
    uintptr_t ctrl_handle_portCTL;

    pthread_t thread0, thread1;

    //my_timer_value1.tv_nsec = 1000;
        my_timer_value1.tv_nsec = 1000;
        my_timer_value1.tv_sec = 0;

        my_timer_value2.tv_nsec = 10000000;
        my_timer_value2.tv_sec = 0;

    timerThread = pthread_create( &thread0, NULL, pulse_to_sensor, (void*) NULL);
    timerThread1 = pthread_create( &thread1, NULL, timer, (void*) NULL);
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
    int detect_flag = 0;
   // loop until there is a key press to stop
    while(1){
        if ((in8(ctrl_handle_portA) & 0x01) && detect_flag == 0)
        {
                start_time = timer_count;
                detect_flag = 1;
                //printf("Sensor is sending something.\n");
        }
        while(in8(ctrl_handle_portA) & 0x01){
                //printf("doing nothing\n");
                        detect_flag = 0;
        }
        end_time = timer_count;
        diff = end_time - start_time;
        if((diff<18000000 && diff>100) && (detect_flag == 0))
        {
                if(diff< min_distance)
                        min_distance = diff;
                if(diff > max_distance)
                        max_distance = diff;
            printf("Distance is : %f\n\r", (double)(SPEED_OF_LIGHT * diff/2));
        }
        else{
//                printf("#####\r");
        }
    }
    pthread_join( thread0, NULL);
    pthread_join( thread1, NULL);

    return 0;
}

void *timer( void *ptr )
{
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
   itime.it_value.tv_nsec = 1000;
   itime.it_interval.tv_sec = 0;
   itime.it_interval.tv_nsec = 1000;

   timer_settime(timer_id, 0, &itime, NULL);

   /*
    * As of the timer_settime(), we will receive our pulse
    * in 1u seconds (the itime.it_value) and every 1u
    * seconds thereafter (the itime.it_interval)
    */

   for (;;) {
       rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
       timer_count++;
   }
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
