/* Libraries for printf, malloc atoi purposes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Libraries for contiki and rime protocol */
#include "contiki.h"
#include "net/rime.h"
//#include "net/rime/announcement.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"

/* Library for restarting the mote */
#include "dev/watchdog.h"

/* Libraries for Z1 mote */
#include "dev/button-sensor.h"
#include "dev/leds.h"
/* Library for 802.15.04 radio */
#include "dev/cc2420.h"
/* Library for temperature sensor */
#include "dev/tmp102.h"
/*Library for battery sensor */
#include "dev/battery-sensor.h"

/* Library included from studentportalen.uu.se example */
#include "clicker.h"

/* GLOBAL VARIABLES */

/* Declaration of variable of how often would read the mote the temperature and send it */
//#define TMP102_READ_INTERVAL (CLOCK_SECOND * 300)

#define TMP102_READ_INTERVAL_MIN (CLOCK_SECOND * 60)
#define TMP102_READ_INTERVAL (TMP102_READ_INTERVAL_MIN * 5)
//#define TMP102_READ_INTERVAL (CLOCK_SECOND * 5)

/* Declaration and assignment of maximum neighbours and total amount of timeout */
#define NEIGHBOR_TIMEOUT 120 * CLOCK_SECOND
#define NEIGHBOR_FORWARD_TIMEOUT 3000 * CLOCK_SECOND
#define MAX_NEIGHBORS 16

/* Holds the number of packets received. */
static int count = 0;



/* Declare etimer for using during the calculation and sending of temperature */ 
/* (for the delay_until function) */
static struct etimer et;
/* Declare timer for announcements */
static struct etimer at;
/* Declare timer for watchdog */
static struct etimer wt;

/* Struct of next neighbor */
struct example_neighbor {
  	struct example_neighbor *next;
  	rimeaddr_t addr;
	uint8_t layer;
  	struct ctimer ctimer;
};

struct forward_neighbor {
  	struct forward_neighbor *next;
  	rimeaddr_t dest_addr;
	rimeaddr_t forward_addr;
  	struct ctimer ctimer;
};

/* Declare our "main" process, the client process*/
PROCESS(client_process, "Stockholm group");
/* Declare our listening announcement process */
PROCESS(anouncement_process, "Anouncenement process");
/* Declare our watchdog process */
PROCESS(watchdog_process, "Watchdog process");
/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process);

/* Compare rime addresses (only the first field) WARNING: THIS DOES NOT WORK */
int rimeaddr_cmp_u80(const rimeaddr_t *addr1, const rimeaddr_t *addr2)
{
	if(addr1->u8[0] != addr2->u8[0]) {
		return 1;
	}
	return 0;
}

/* Compare rime addresses (only the second field) WARNING: THIS DOES NOT WORK */
int rimeaddr_cmp_u81(const rimeaddr_t *addr1, const rimeaddr_t *addr2)
{
	if(addr1->u8[1] > addr2->u8[1]) {
		return 1;
	}
	else if (addr1->u8[1] < addr2->u8[1]) {
		return -1;
	}
	return 0;
}

/* Declare neighbor table list */
LIST(neighbor_table);
MEMB(neighbor_mem, struct example_neighbor, MAX_NEIGHBORS);
/* Declare forward table list */
LIST(forward_table);
MEMB(forward_mem, struct forward_neighbor, MAX_NEIGHBORS);

uint8_t layer = 50;

/* Multihop functions */

/*
 * This function is called by the ctimer present in each neighbor
 * table entry. The function removes the neighbor from the table
 * because it has become too old.
 */
static void remove_neighbor(void *n)
{
  	struct example_neighbor *e = n;
	layer = 50;
  	list_remove(neighbor_table, e);
  	memb_free(&neighbor_mem, e);
}

static void remove_forward_neighbor(void *n)
{
  	struct forward_neighbor *e = n;

  	list_remove(forward_table, e);
  	memb_free(&forward_mem, e);
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

/*
 * This function is called at the final recepient of the message.
 */
static void recv(struct multihop_conn *c, const rimeaddr_t *from, 
	const rimeaddr_t *prevhop, uint8_t hops)
{
	//If received message is RESET
	if( strcmp((char *) packetbuf_dataptr(), "RESET\n") ) {
		printf("\nI RECEIVED MESSAGE FROM BASESTATION! IF STATEMENT\n\n");
		etimer_set(&wt, CLOCK_SECOND*3600);
	}
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
	printf("IM INSIDE FORWARDING!\n");
	struct example_neighbor *n;
	//Set the desired destination
	rimeaddr_t destination;
	destination.u8[0] = 128;
	destination.u8[1] = 0;
	/* Declare forward_neighbor struct for updating forward_table list */
	struct forward_neighbor *e;
	int flag_forward = 0;
	/* Declare NULL rime address */
	rimeaddr_t null_address;
	null_address.u8[0] = 0;
	null_address.u8[1] = 0;
	//if destination is the same as dest
	if( rimeaddr_cmp(&destination, dest) )
	{
		//Control if originator is not the same as current mote (for updating the forward_table list)
		if( !rimeaddr_cmp(&rimeaddr_node_addr, originator) ) {
			//Update forward_table list
			//Loop through the forward table list
			printf("\n\nFORWARD NEIGHBOR IF STATEMENT\n\n");
			if (list_length(forward_table) > 0) {
				for(e = list_head(forward_table); e != NULL && flag_forward != 0; e = e->next) {
					//if address is the same as originator (the originator exists already in the list)
					if( rimeaddr_cmp(&e->dest_addr, originator) ) {
						printf("FORWARD NEIGHBOUR WAS FOUND! UPDATE TIMER\n");
						//Update neighbor forward timer
						//if prevhop is 0.0 (no previous hops applied)
						if( rimeaddr_cmp(&null_address, prevhop) ) {
							//set forward_addr to originator
							rimeaddr_copy(&e->forward_addr, originator);
						}
						else { //else, set the forward address to previous hop
							rimeaddr_copy(&e->forward_addr, prevhop);
						}
						
						ctimer_set(&e->ctimer, NEIGHBOR_FORWARD_TIMEOUT, remove_forward_neighbor, e);
						//Set flag forward to 1
						flag_forward = 1;
					}
				}
			}
			//if flag_forward is still 0 (did not found the forward neighbor)
			if( flag_forward == 0) {
				/* The neighbor was not found in the list, so we add a new entry by
				allocating memory from the neighbor_mem pool, fill in the
				necessary fields, and add it to the list. */
				e = memb_alloc(&forward_mem);
				if(e != NULL) {
					printf("FORWARD NEIGHBOR WAS NOT FOUND! SET NEW FORWARD NEIGHBOUR TO LIST\n");
					
					rimeaddr_copy(&e->dest_addr, originator);
					printf("e->dest_addr: %d.%d\n", e->dest_addr.u8[0], e->dest_addr.u8[1]);
					
					//if prevhop is 0.0 (no previous hops applied)
					if( rimeaddr_cmp(&null_address, prevhop) ) {
						//set forward_addr to originator
						rimeaddr_copy(&e->forward_addr, originator);
					}
					else { //else, set the forward address to previous hop
							rimeaddr_copy(&e->forward_addr, prevhop);
					}
					printf("e->forward_addr: %d.%d\n", e->forward_addr.u8[0], e->forward_addr.u8[1]);	
					list_add(forward_table, e);
					ctimer_set(&e->ctimer, NEIGHBOR_FORWARD_TIMEOUT, remove_forward_neighbor, e);
				}
				
			}
		}
		//Forward the message to destination
		if(list_length(neighbor_table) > 0) {
			//Loop through the list for getting layer 0
			for(n = list_head(neighbor_table); n != NULL; n = n->next) {
				printf("LOOP THROUGH NEIGHBOURS! %d.%d\n", n->addr.u8[0], n->addr.u8[1]);
				//If rime address is the destination
				if(rimeaddr_cmp(&destination, &n->addr)) {
					//Break the loop and get the desired n value
					//printf("Found the 128.0 address! Setting num to %d! \n", pointer);
					printf("Found destination! Forwarding message to destination\n");
					return &n->addr;
				}
			}
			//if flag is still 0, loop through the other motes
			for(n = list_head(neighbor_table); n != NULL; n = n->next)
			{
				//if neighbour address is closer to the basestation
				if( n->layer < layer && !rimeaddr_cmp(&destination, &n->addr) )
				{
					printf("Did not found destination! Forwarding message to: %d.%d\n", n->addr.u8[0], n->addr.u8[1]);
					//Break the loop and get the desired n value
					return &n->addr;
				}
			}
  		}
		
	}
	//else if destination is not the basestation
	else {
		printf("\n\nDestination is not the basestation!\n\n");
		if(list_length(forward_table) > 0) {
			//Loop through the list
			printf("There are items in forward table\n");
			for(e = list_head(forward_table); e != NULL; e = e->next) {
				printf("Running through the forward_table loop\n");
				printf("Variable e->dest_addr: %d.%d\n", e->dest_addr.u8[0], e->dest_addr.u8[1]);
				printf("Variable e->forward_addr: %d.%d\n", e->forward_addr.u8[0], e->forward_addr.u8[1]);
				printf("Variable dest: %d.%d\n", dest->u8[0], dest->u8[1]);
				//if destination is inside the list
				if ( rimeaddr_cmp(&e->dest_addr, dest) ) {
					printf("Found forward destination! Forwarding message to prevhop!\n");
					//forward the message to forward address
					return &e->forward_addr;
				}
			}
		}
	}
	
	
	printf("%d.%d: did not find a neighbor to forward to\n", rimeaddr_node_addr.u8[0], 
	rimeaddr_node_addr.u8[1]);
	
	return NULL;
}
static const struct multihop_callbacks multihop_call = {recv, forward};
static struct multihop_conn multihop;

/* Broadcast functions */

static struct broadcast_conn broadcast;

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	//Declare destination
	rimeaddr_t destination;
	destination.u8[0] = 128;
	destination.u8[1] = 0;
	
	//Declare rss variables
	static signed char rss;
  	static signed char rss_val;
  	static signed char rss_offset;
	
	

	printf("broadcast message received from %d.%d: %s\n", from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
	
	//Get the RSSI (signal strength)
	rss_val = cc2420_last_rssi;  // Get the RSSI from the last received packet
	rss_offset = -45; // Datasheet of cc2420 page 49 says you must decrease the value in -45
	rss = rss_val + rss_offset; // The RSSI correct value in dBm

	//Print signal strength

	printf("RSSI: %d\n", rss);
	
	//If rss is less than -79 (neighbor is too far away)
	if(rss <= -79)
	{
		//Stop the function
		return;
	}

	//if broadcast is being received from basestation
	if(rimeaddr_cmp(from, &destination))
	{
		layer = 1;
	}
	//else, if your layer is greater than the received layer 
	else if(layer > atoi((char *)packetbuf_dataptr()))
	{
		layer = atoi((char *)packetbuf_dataptr()) + 1;

	}
	struct example_neighbor *e;
	//declare flag for stopping the if statements
	int flag = 0;
	/* We received an announcement from a neighbor so we need to update
		the neighbor list, or add a new entry to the table. */
	
	for(e = list_head(neighbor_table); e != NULL; e = e->next) {
		printf("DEBUGGING IM CHECKING THE NEIGHBOURS\n");
		//Store the current loop variable to loop_variable
		//rimeaddr_copy (&e->addr,  &loop_variable);
		//printf("LOOP_VARIABLE IS: %d.%d\n", loop_variable.u8[0], loop_variable.u8[1]);
		
		// if the neighbour is the same as the basestation
		if(rimeaddr_cmp(from, &destination)) {
			printf("FOUND NEIGHBOUR BASESTATION!\n");
			leds_on(LEDS_GREEN);
			leds_off(LEDS_BLUE);
			//printf("My new address is: %d.%d!\n", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);	
			flag = 1;
			//update current variable
		}

		if(rimeaddr_cmp(from, &e->addr) ) {
		/* Our neighbor was found, we update the current neighbour */
			printf("NEIGHBOUR WAS FOUND! UPDATE TIMER\n");
		/* Our neighbor was found, so we update the timeout. */
			printf("UPDATE LAYER\n");
			//Update the layer
			e->layer = atoi((char *)packetbuf_dataptr());
			ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
			printf("STOP FUNCTION\n");
      		return;
    		}		
  	}
	for(e = list_head(neighbor_table); e != NULL && flag == 0; e = e->next) {
		//if flag is still 0 (the mote cannot find the basestation) and the 2nd address (.0, .1 etc) is less than the current address OR current address's 2nd address (.0, .1 etc) is 0 (INITIAL VALUE)
		if( !rimeaddr_cmp(from, &destination) ){
			printf("FOUND ANOTHER NEIGHBOUR!\n");
			//temp_variable.u8[1] = e->addr.u8[1] + 1;
			//Update mote's current address 
			//rimeaddr_set_node_addr(&temp_variable);
			//printf("My new address is: %d.%d!\n", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
			leds_off(LEDS_GREEN);
			leds_on(LEDS_BLUE);
		}
		if(rimeaddr_cmp(from, &e->addr) ) {
		/* Our neighbor was found, we update the current neighbour */
			printf("NEIGHBOUR WAS FOUND! UPDATE TIMER\n");
		/* Our neighbor was found, so we update the timeout. */
			printf("UPDATE LAYER\n");
			e->layer = atoi((char *)packetbuf_dataptr());
			ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
			printf("STOP FUNCTION\n");
      		return;
    		}
	}

	/* The neighbor was not found in the list, so we add a new entry by
		allocating memory from the neighbor_mem pool, fill in the
		necessary fields, and add it to the list. */
	e = memb_alloc(&neighbor_mem);
	if(e != NULL) {
		printf("NEIGHBOUR WAS NOT FOUND! SET NEW NEIGHBOUR TO LIST\n");
		printf("FROM VARIABLE: %d.%d\n", from->u8[0], from->u8[1]);
		e->layer = atoi((char *)packetbuf_dataptr());
		rimeaddr_copy(&e->addr, from);
		list_add(neighbor_table, e);
		ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
	}
	
}


static const struct broadcast_callbacks broadcast_call = {broadcast_recv};


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
	
	/* Declaration of temperature variables */
	int16_t tempint;
	uint16_t tempfrac;
	int16_t raw[1];
	char str_raw[10];
	uint16_t absraw;
	int16_t sign;
	char minus = ' ';
	
	/* Battery value */
	uint16_t bateria;
	/* Initialize temperature */
	tmp102_init();
	
	/* Activate the button sensor. */
	SENSORS_ACTIVATE(button_sensor);
	/* Activate the battery sensor. */
	SENSORS_ACTIVATE(battery_sensor);
	/* Set the radio's channel to IEEE802_15_4_CHANNEL */
	cc2420_set_channel(IEEE802_15_4_CHANNEL);
	
	/* Set the radio's transmission power. */
	cc2420_set_txpower(CC2420_TX_POWER);
	
	/* Start anouncement process */
	process_start(&anouncement_process, "");
	
	/* Start watchdog process */
	process_start(&watchdog_process, "");

	rimeaddr_t to;
		
	
	/* Loop forever. */
	while (1) {
		
		/* Wait until an event occurs. If the event has
		 * occured, ev will hold the type of event, and
		 * data will have additional information for the
		 * event. In the case of a sensors_event, data will
		 * point to the sensor that caused the event.
		 * Here we wait until the button was pressed. */
		/* PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
			data == &button_sensor); */
		/*wait READ_INTERVAL */
		etimer_set(&et, TMP102_READ_INTERVAL);
    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		/*Get the battery value*/
		bateria = battery_sensor.value(0);
		/*Get the raw data of the temp */		
		raw[0] = tmp102_read_temp_raw();
		leds_toggle(LEDS_RED);
		//printf("Client: Raw data = %d\n", *raw);
		snprintf( str_raw, 10, "%d,%i,", raw[0], bateria);
		/* Copy the str_raw into the packet buffer. */
		packetbuf_copyfrom(str_raw, 10);
		/* Set the Rime address of the final receiver of the packet to
		1.0. This is a value that happens to work nicely in a Cooja
		simulation (because the default simulation setup creates one
		node with address 1.0). */
		
		to.u8[0] = 128;
		to.u8[1] = 0;
		
		if(rimeaddr_node_addr.u8[0] == to.u8[0]){
			sign = 1;
			absraw = raw[0];
			if(raw[0] < 0) {// Perform 2C's if sensor returned negative data
				absraw = (*raw ^ 0xFFFF) + 1;
				sign = -1;
			}
			tempint = (absraw >> 8) * sign;
			tempfrac = ((absraw >> 4) % 16) * 625;	// Info in 1/10000 of degree
    			
			minus = ((tempint == 0) & (sign == -1)) ? '-' : ' ';

			printf("Basestation: No need to send message! Count: %d\n", count);
			printf("Basestation: Address: %d.%d: Temp = %c%d.%04d \n",
			        rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], minus, tempint, tempfrac);
		}
		else
		{
			/* Send the packet. */
			printf("Trying to send message to basestation!\n");
			multihop_send(&multihop, &to);
		}
	}

	/* This will never be reached, but we'll put it here for
	 * the sake of completeness: */
	PROCESS_END();
}

/* Our announcement process */
PROCESS_THREAD(anouncement_process, ev, data) {
	//PROCESS_EXITHANDLER(multihop_close(&multihop);)
	
	PROCESS_BEGIN();
	broadcast_open(&broadcast, 129, &broadcast_call);
	
	char str_send[3];
	/* Set announcement timer to 10 seconds */
	while(1)
	{
		
		//printf("Listening announcement!\n");
		etimer_set(&at, CLOCK_SECOND*60);
		PROCESS_WAIT_UNTIL(etimer_expired(&at));
		snprintf(str_send, 3, "%d\n", layer);
		printf("Broadcasting: %s \n", str_send);
		packetbuf_copyfrom(str_send, 3);
		broadcast_send(&broadcast);
		
	}
	PROCESS_END();
}

/* Our watchdog process */
PROCESS_THREAD(watchdog_process, ev, data) {
//	PROCESS_EXITHANDLER(multihop_close(&multihop);)
	
	PROCESS_BEGIN();
	while(1)
	{
		printf("Watchdog started!\n");
		etimer_set(&et, CLOCK_SECOND*3600);
		PROCESS_WAIT_UNTIL(etimer_expired(&et));
		/* Reboot mote */
		watchdog_reboot();
		
	}
	PROCESS_END();
}
