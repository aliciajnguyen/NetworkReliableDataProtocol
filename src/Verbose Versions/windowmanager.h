/* This is the HEADER FILE for the window manager for acknowledged packets in cli.c
*/
#ifndef WINDOWMANAGER_H_ 
#define WINDOWMANAGER_H_

//VARIABLES
extern int thread_state;
extern pthread_mutex_t win_x;

//FUNCTION DECLARATION

//#############################################################################################
//THREAD FUNCTIONS
//#############################################################################################
//main function to manage timers using a thread created in cli.c
//while the global var is set to not shut down
//thread updates timers in timer_update
//if there are expired timers in window, thread resends and restarts timer
void thread_timer_actions();

//function updates all rdp_pkt structs in window
//updates timer values based on time
//executed by thread 
void timer_update();

//handler for time_out events
//a function that checks for timed-out packets in the window
//if a packet timer is out (indicated in time_left of the rdp_pkt struct)
//calls udp_send using info from rdp_pkt in window
//also restarts the timer
void timeout_event_handler();

//#############################################################################################
//UTILITY FUNCTIONS
//#############################################################################################
//when a packet is sent must be added to list of unacked packets (window)
//"window" = list of unacked packets w timers, awaiting acking receipt
//creates rdp_pkt structure, adds to list
// undefined behaviour if window overflows
// window size defined as constant WINDOW_SIZE
int add_pkt_window(struct udp_pcb *pcb, struct pbuf *p);

//upon receipt of an ack, delete_pkt is called to remove an entry a packet
//from the window of packets awaiting an ack
//delete packet by freeing memory and reiniting to 0 so we know it's empty
//if del_pkt called there should be a packet awaiting an ack
//if no packet found, returns an error
int del_pkt_window(struct udp_pcb *pcb, struct pbuf *p);

//#############################################################################################
//HELPER FUNCTIONS
//#############################################################################################
//creates a rdp_pkt struct by copying from current pbuf
//this struct is entry placed in window for ack waiting
//returns a pointer to it
//ensured to deep copy payload so can match later
//memory allocated here freed in del_pkt
struct rdp_pkt * create_pkt(struct udp_pcb *pcb, struct pbuf *p);

//helper function to find a matching packet in window
//check if rdp_pkt is a match based on payload
//logic is that if you ack the wrong packet bc same payload doesnt really matter
int pkt_match(struct udp_pcb *pcb, struct pbuf *p, struct rdp_pkt * packet);

//helper function to print window for debug
void print_window();


#endif