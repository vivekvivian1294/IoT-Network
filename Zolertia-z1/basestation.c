/* Libraries for printf, malloc atoi purposes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Libraries for contiki and rime protocol */
#include "contiki.h"
#include "net/rime.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"

/* Libraries for Z1 mote */
#include "dev/button-sensor.h"
#include "dev/leds.h"
/* Library for 802.15.04 radio */
#include "dev/cc2420.h"
/* Library for temperature Sensor */
#include "dev/tmp102.h"

/* Library included from studentportalen.uu.se example */
#include "clicker.h"

/* Library for restarting the mote */
#include "dev/watchdog.h"

/* Library for energy */
#include "sys/energest.h"

/* GLOBAL VARIABLES */

/* Declaration of variable of how often would read the mote the temperature and send it */
#define TMP102_READ_INTERVAL (CLOCK_SECOND * 2)

/* Declaration and assignment of maximum neighbours and total amount of timeout */
#define NEIGHBOR_TIMEOUT 3000 * CLOCK_SECOND
#define MAX_NEIGHBORS 16

/* Holds the number of packets received. */
static int count = 0;

/* Function for using the battery sensor */
float
floor(float x)
{
  if(x >= 0.0f) {
    return (float) ((int) x);
  } else {
    return (float) ((int) x - 1);
  }
}

/* Declare etimer for using during the calculation and sending of temperature */ 
/* (for the delay_until function) */
static struct etimer et;

/* Declare timer for announcements */

static struct etimer at;

/* Declare rebooting timer */
static struct etimer etimer;

/* Struct of next neighbor */
struct example_neighbor {
  	struct example_neighbor *next;
  	struct ctimer ctimer;
	rimeaddr_t forward_addr;
	rimeaddr_t dest_addr;
};

/* Declare our "main" process, the client process*/
PROCESS(client_process, "Stockholm group");
/* Declare our listening announcement process */
PROCESS(anouncement_process, "Anouncenement process");
/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process);

/* Multihop functions */

/* List for the neighbor table */
LIST(neighbor_table);

MEMB(neighbor_mem, struct example_neighbor, MAX_NEIGHBORS);

/* Broadcast functions */
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*
 * This function is called by the ctimer present in each neighbor
 * table entry. The function removes the neighbor from the table
 * because it has become too old.
 */
static void remove_neighbor(void *n)
{
  	struct example_neighbor *e = n;
  	list_remove(neighbor_table, e);
	memb_free(&neighbor_mem, e);

}

/*
 * This function is called when an incoming announcement arrives. The
 * function checks the neighbor table to see if the neighbor is
 * already present in the list. If the neighbor is not present in the
 * list, a new neighbor table entry is allocated and is added to the
 * neighbor table.
 */
static void received_announcement(struct announcement *a, 
	const rimeaddr_t *from,	uint16_t id, uint16_t value)
{
	
}
static struct announcement example_announcement;

/* Declare multihop variable */
static struct multihop_conn multihop;

/*
 * This function is called at the final recepient of the message.
 */
static void recv(struct multihop_conn *c, const rimeaddr_t *from, 
	const rimeaddr_t *prevhop, uint8_t hops)
{
	struct example_neighbor *n;
	int loop_flag = 0;
	//if from is not the same as previous hop
	/* Update your forward list, if you want to send a message back to the originator you forward it to prevhop */
	//if there are already items in the table
	if(list_length(neighbor_table) > 0) {
		//Loop through the forward table
		//printf("LOOP THROUGH LIST\n");
		for(n = list_head(neighbor_table); n != NULL && loop_flag == 0; n = n->next) {
			//if fn->dest_addr is the same as from address (address is already in our list 
			if( rimeaddr_cmp(&n->dest_addr, from) ) {
				//Break for loop
				//printf("FOUND ADDRESS!\n");
				//Update forward address
				rimeaddr_copy(&n->forward_addr, prevhop);
				loop_flag = 1;
				ctimer_set(&n->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, n);
			}
		}
	}
	
	//if loop_flag is still 0 (address was not found)
	if(loop_flag == 0) {
		//Loop again through the list 
		//printf("ADDRESS WAS NOT FOUND! LOOP AGAIN THROUGH LIST!\n");
		for(n = list_head(neighbor_table); n != NULL && loop_flag == 0; n = n->next);
		//Allocate new memory
		n = memb_alloc(&neighbor_mem);
		if(n != NULL) {
			//Add new address to forward list
			rimeaddr_copy(&n->dest_addr, from);
			rimeaddr_copy(&n->forward_addr, prevhop);
			list_add(neighbor_table, n);
		}
			
	}
	/* Update rebooting timer to 3600 seconds */
	//printf("SET NEW TIMER\n");
	etimer_set(&etimer, CLOCK_SECOND*3600);
	//Initialize counter
	count++;
	//Initialize i for for loop
	int i = 0;
	/* Declaration of str_raw message that is received */
	char *str_raw;
	
	/* Declaration of temperature variables */
	char str_raw_temp[40];
	int str_raw_temp_counter = 0;
	int16_t tempint;
	uint16_t tempfrac;
	int16_t raw[1];
	uint16_t absraw;
	int16_t sign;
	char minus = ' ';
	
	/* Declaration of battery variables */
	uint16_t bateria;
	char str_bateria[40];
	int str_raw_batt_counter = 0;

	/* Boolean variable for toggling which string should assign */
	int flag_for_string = 0;
	/* 0bxxxxx allows us to write binary values */
	/* for example, 0b10 is 2 */
	/* Allocate memory for str_raw*/
	str_raw = malloc(sizeof(40));

	/* The packetbuf_dataptr() returns a pointer to the first data byte
     	in the received packet. */
	/* Get the raw data */
	str_raw = packetbuf_dataptr();
	
	//Get the string output for the temp and the battery
	for (i = 0; i < strlen(str_raw); i++)
	{
		if(str_raw[i] != ',' && flag_for_string == 0)
		{
			str_raw_temp[str_raw_temp_counter] = str_raw[i];
			str_raw_temp_counter++;
		}
		else if(str_raw[i] == ',' && flag_for_string == 0)
		{
			flag_for_string = 1;
		}
		else if(str_raw[i] != ',' && flag_for_string == 1)
		{
			str_bateria[str_raw_batt_counter] = str_raw[i];
			str_raw_batt_counter++;
		}
		else if(str_raw[i] == ',' && flag_for_string == 1)
		{
			flag_for_string = 2;
		}
		else if(flag_for_string = 2)
		{
			break;
		}
	}
	//After loop, set counters and flag to 0
	str_raw_temp_counter = 0;
	str_raw_batt_counter = 0;
	flag_for_string = 0;
	
	//get the temp and battery variables
	raw[0] = atoi(str_raw_temp);
	bateria = atoi(str_bateria);

	//Run another loop to reset the string outputs
	for(i = 0; i < 40; i++)
	{
		str_raw_temp[i] = 0;
		str_bateria[i] = 0;
	} 
	
	//Set the temperature variables
	sign = 1;
	absraw = raw[0];
    	if(raw[0] < 0) {	// Perform 2C's if sensor returned negative data
      		absraw = (raw[0] ^ 0xFFFF) + 1;
      		sign = -1;
    	}
    	tempint = (absraw >> 8) * sign;
    	tempfrac = ((absraw >> 4) % 16) * 625;	// Info in 1/10000 of degree
    	minus = ((tempint == 0) & (sign == -1)) ? '-' : ' ';

	//Set the battery variables
	float mv = (bateria * 2.500 * 2) / 4096;

	//Printf for serial
	printf("Temp=%c%d.%04d,Battery=%ld.%03d,Mote=%d.%d\n", minus, tempint, tempfrac, (long) mv, (unsigned) ((mv - floor(mv)) * 1000), from->u8[0], from->u8[1]);
	
	//Send back the message for resetting the watchdog of client
	packetbuf_copyfrom("RESET", 6);
	multihop_send(&multihop, from);
	//printf("Basestation: Message received! Count: %d\n", count);
	//printf("Multihop message received from %d.%d: Temp = %c%d.%04d \n",
        //from->u8[0], from->u8[1], minus, tempint, tempfrac);
	printf("Previous hop: %d.%d\n", prevhop->u8[0], prevhop->u8[1]);
	leds_off(LEDS_ALL);
	leds_on(count & 0b111);
	free(str_raw);
	//printf("STR_RAW IS FREE!\n");
}

/*
 * This function is called to forward a packet. The function picks a
 * random neighbor from the neighbor list and returns its address. The
 * multihop layer sends the packet to this address. If no neighbor is
 * found, the function returns NULL to signal to the multihop layer
 * that the packet should be dropped.
 */



static rimeaddr_t *
forward(struct multihop_conn *c, const rimeaddr_t *originator, 
	const rimeaddr_t *dest, const rimeaddr_t *prevhop, 
	uint8_t hops)
{
	
	//printf("IM INSIDE FORWARDING!\n");
	struct example_neighbor *n;
	//Set the desired destination
	rimeaddr_t destination;
	destination.u8[0] = 128;
	destination.u8[1] = 0;
	//If destination is not the basestation
	if( !rimeaddr_cmp(dest, &destination) ) {
		//printf("Destination is not the basestation!\n");
		if(list_length(neighbor_table) > 0) {
			//Loop through the forward list
			//printf("LOOPING THROUGH THE FORWARD LIST");
			for(n = list_head(neighbor_table); n != NULL; n = n->next) {
				//if destination address is the same as n->dest_addr 
				if( rimeaddr_cmp(&n->dest_addr, dest) ) {
					//Forward message to forward_addr
					printf("FORWARD MESSAGE TO %d.%d\n", n->forward_addr.u8[0], n->forward_addr.u8[1]);
					return &n->forward_addr;
				}
			}
		}
		
	}

	/*
	printf("%d.%d: did not find a neighbor to forward to\n", rimeaddr_node_addr.u8[0], 
	rimeaddr_node_addr.u8[1]);
	*/
	return NULL;
}

/* Declare multihop callbacks */
static const struct multihop_callbacks multihop_call = {recv, forward};
/* Our main process. */
PROCESS_THREAD(client_process, ev, data) {
	//PROCESS_EXITHANDLER(multihop_close(&multihop);)
	
	PROCESS_BEGIN();
	
	/* Initialize the memory for the neighbor table entries. */
	memb_init(&neighbor_mem);

	/* Initialize the list used for the neighbor table. */
	list_init(neighbor_table);

	/* Open a multihop connection on Rime channel CHANNEL. */
	multihop_open(&multihop, CLICKER_CHANNEL, &multihop_call);
	
	/* Register an announcement with the same announcement ID as the
	Rime channel we use to open the multihop connection above. */
	announcement_register(&example_announcement, CLICKER_CHANNEL, 
		received_announcement);

	/* Set a dummy value to start sending out announcments. */
	announcement_set_value(&example_announcement, 0);
	
	
	/* Initialize temperature */
	tmp102_init();
	
	/* Activate the button sensor. */
	SENSORS_ACTIVATE(button_sensor);
	/* Set the radio's channel to IEEE802_15_4_CHANNEL */
	cc2420_set_channel(IEEE802_15_4_CHANNEL);
	
	/* Set the radio's transmission power. */
	cc2420_set_txpower(CC2420_TX_POWER);
	
	/* Start anouncement process */
	process_start(&anouncement_process, "");

	
	/* Set rebooting timer to 3600 seconds */
	etimer_set(&etimer, CLOCK_SECOND*3600);
	
	PROCESS_WAIT_UNTIL(etimer_expired(&etimer));
	/* Reboot mote */
	//printf("REBOOTING MOTE!\n");
	watchdog_reboot();
	

	PROCESS_END();
}

/* Our announcement process */
PROCESS_THREAD(anouncement_process, ev, data) {
	//PROCESS_EXITHANDLER(multihop_close(&multihop);)
	static int counter = 0;
	
	PROCESS_BEGIN();
	
	broadcast_open(&broadcast, 129, &broadcast_call);
	
	/* Set announcement timer to 10 seconds */
	while(1)
	{
		//printf("Listening announcement!\n");
		etimer_set(&at, CLOCK_SECOND*2);
		PROCESS_WAIT_UNTIL(etimer_expired(&at));
		
		packetbuf_copyfrom("Hello", 6);
    		broadcast_send(&broadcast);
		if(counter > 1000){counter = 0;}
		//announcement_set_value(&example_announcement, counter);
		counter++;
	}
	PROCESS_END();
}
