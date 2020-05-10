/* Libraries for printf, malloc atoi purposes */
#include <stdio.h>
#include <stdlib.h>

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
/* Library for temperature sensor */
#include "dev/tmp102.h"
/*Library for battery sensor */
#include "dev/battery-sensor.h"

/* Library included from studentportalen.uu.se example */
#include "clicker.h"

/* GLOBAL VARIABLES */

/* Declaration of variable of how often would read the mote the temperature and send it */
#define TMP102_READ_INTERVAL (CLOCK_SECOND * 300)
//#define TMP102_READ_INTERVAL (CLOCK_SECOND * 5)

/* Declaration and assignment of maximum neighbours and total amount of timeout */
#define NEIGHBOR_TIMEOUT 1800 * CLOCK_SECOND
#define MAX_NEIGHBORS 16

/* Holds the number of packets received. */
static int count = 0;



/* Declare etimer for using during the calculation and sending of temperature */ 
/* (for the delay_until function) */
static struct etimer et;

/* Struct of next neighbor */
struct example_neighbor {
  	struct example_neighbor *next;
  	rimeaddr_t addr;
  	struct ctimer ctimer;
};

/* Declare our "main" process, the client process*/
PROCESS(client_process, "Stockholm group");
/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process);


/* Multihop functions */

LIST(neighbor_table);
MEMB(neighbor_mem, struct example_neighbor, MAX_NEIGHBORS);

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
	struct example_neighbor *e;

	/*  printf("Got announcement from %d.%d, id %d, value %d\n",
		from->u8[0], from->u8[1], id, value);*/

	/* We received an announcement from a neighbor so we need to update
		the neighbor list, or add a new entry to the table. */
	for(e = list_head(neighbor_table); e != NULL; e = e->next) {
		if(rimeaddr_cmp(from, &e->addr)) {
		/* Our neighbor was found, so we update the timeout. */
			ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
      		return;
    		}
  	}

	/* The neighbor was not found in the list, so we add a new entry by
		allocating memory from the neighbor_mem pool, fill in the
		necessary fields, and add it to the list. */
	e = memb_alloc(&neighbor_mem);
	if(e != NULL) {
		rimeaddr_copy(&e->addr, from);
		list_add(neighbor_table, e);
		ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
	}
}
static struct announcement example_announcement;

/*
 * This function is called at the final recepient of the message.
 */
static void recv(struct multihop_conn *c, const rimeaddr_t *from, 
	const rimeaddr_t *prevhop, uint8_t hops)
{
	count++;
	
	/* Declaration of temperature variables */
	int16_t tempint;
	uint16_t tempfrac;
	int16_t raw[1];
	char *str_raw;
	uint16_t absraw;
	int16_t sign;
	char minus = ' ';

	/* 0bxxxxx allows us to write binary values */
	/* for example, 0b10 is 2 */
	

	/* The packetbuf_dataptr() returns a pointer to the first data byte
     	in the received packet. */
	/* Get the raw data */
	str_raw = packetbuf_dataptr();
	raw[0] = atoi(str_raw);
	sign = 1;
	absraw = raw[0];
    	if(raw[0] < 0) {	// Perform 2C's if sensor returned negative data
      		absraw = (*raw ^ 0xFFFF) + 1;
      		sign = -1;
    	}
    	tempint = (absraw >> 8) * sign;
    	tempfrac = ((absraw >> 4) % 16) * 625;	// Info in 1/10000 of degree
    	minus = ((tempint == 0) & (sign == -1)) ? '-' : ' ';
	
	printf("Basestation: Message received! Count: %d\n", count);
	printf("Multihop message received from %d.%d: Temp = %c%d.%04d \n",
        from->u8[0], from->u8[1], minus, tempint, tempfrac);
	printf("Previous hop: %d.%d\n", prevhop->u8[0], prevhop->u8[1]);
	leds_off(LEDS_ALL);
	leds_on(count & 0b111);
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
	/* Find a random neighbor to send to. */
	int num = 0, i;
	int flag = 0;
	int pointer = 0;
	struct example_neighbor *n;
	//Set the desired destination
	rimeaddr_t destination;
	destination.u8[0] = 128;
	destination.u8[1] = 0;
	//Layer one
	rimeaddr_t layer10;
	layer10.u8[0] = 142;
	layer10.u8[1] = 0;

	rimeaddr_t layer11;
	layer11.u8[0] = 140;
	layer11.u8[1] = 0;

	//Layer two
	rimeaddr_t layer20;
	layer20.u8[0] = 42;
	layer20.u8[1] = 0;

	rimeaddr_t layer21;
	layer21.u8[0] = 127;
	layer21.u8[1] = 0;	
	

	if(list_length(neighbor_table) > 0) {
		//Loop through the list for getting layer 0
		for(n = list_head(neighbor_table); n != NULL; n = n->next) {
			//If rime address is the destination
			if(rimeaddr_cmp(&destination, &n->addr)) {
				//Break the loop and get the desired n value
				//printf("Found the 128.0 address! Setting num to %d! \n", pointer);
				num = pointer;
				//Set flag to 1, so no random variables is used
				flag = 1;
			}
		pointer++;
		}
		//Set pointer to 0 to run a new loop
		pointer = 0;
		//if flag is still 0, loop through layer 1
		if(flag == 0)
		{
			for(n = list_head(neighbor_table); n != NULL; n = n->next) {
			//If rime address is the destination
			if(rimeaddr_cmp(&layer10, &n->addr)) {
				//Break the loop and get the desired n value
				//printf("Found the 142.0 address! Setting num to %d! \n", pointer);
				num = pointer;
				//Set flag to 1, so no random variables is used
				flag = 1;
			}
			else if(rimeaddr_cmp(&layer11, &n->addr)) {
				//Break the loop and get the desired n value
				//printf("Found the 140.0 address! Setting num to %d! \n", pointer);
				num = pointer;
				//Set flag to 1, so no random variables is used
				flag = 1;
			}
			pointer++;
			}
		}
		//Set pointer to 0 to run a new loop
		pointer = 0;
		//if flag is still 0, loop through layer 2
		if(flag == 0)
		{
			for(n = list_head(neighbor_table); n != NULL; n = n->next) {
			//If rime address is the destination
			if(rimeaddr_cmp(&layer20, &n->addr)) {
				//Break the loop and get the desired n value
				//printf("Found 42.0 address! Setting num to %d! \n", pointer);
				num = pointer;
				//Set flag to 1, so no random variables is used
				flag = 1;
			}
			else if(rimeaddr_cmp(&layer21, &n->addr)) {
				//Break the loop and get the desired n value
				//printf("Found 127.0 address! Setting num to %d! \n", pointer);
				num = pointer;
				//Set flag to 1, so no random variables is used
				flag = 1;
			}
			pointer++;
			}
		}
		//Set pointer to 0 
		pointer = 0;
		//If flag is still 0, set it to a random neighbour
		if(flag == 0)
		{
			num = random_rand() % list_length(neighbor_table);			
			//printf("Did not found the address! Setting num variable!\n");
		}
		//Set flag and i to 0		
		flag = 0;
		i = 0;
		for(n = list_head(neighbor_table); n != NULL && i != num; n = n->next) {
			++i;
		}
		if(n != NULL) {
			
			/*
			printf("%d.%d: Forwarding packet to %d.%d (%d in list), hops %d\n",
			rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
			n->addr.u8[0], n->addr.u8[1], num,
			packetbuf_attr(PACKETBUF_ATTR_HOPS));
			*/
			return &n->addr;
		}
  	}
	/*
	printf("%d.%d: did not find a neighbor to forward to\n", rimeaddr_node_addr.u8[0], 
	rimeaddr_node_addr.u8[1]);
	*/
	return NULL;
}
static const struct multihop_callbacks multihop_call = {recv, forward};
static struct multihop_conn multihop;



/* Our main process. */
PROCESS_THREAD(client_process, ev, data) {
	PROCESS_EXITHANDLER(multihop_close(&multihop);)
	
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
			multihop_send(&multihop, &to);
		}
	}

	/* This will never be reached, but we'll put it here for
	 * the sake of completeness: */
	PROCESS_END();
}
