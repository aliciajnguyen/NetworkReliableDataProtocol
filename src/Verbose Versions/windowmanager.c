/* This is the window manager for acknowledged packets in cli.c */
#include "cli.h"
#include "udp.h"
#include "windowmanager.h"
#include "sys/time.h"

//CONSTANTS
#define WINDOW_SIZE 10
#define ERR_OK 0
#define ERR_NOTOK 1
#define TIMEOUT_MS 500 //time for timeout in us (500 ms or 0.5 s) 
#define THREAD_SLEEP_TIME 10
#define THREAD_RUN = 1
#define THREAD_CANCEL = 0

//VARIABLES
struct rdp_pkt{
    struct udp_pcb * pcb;
    struct pbuf * p;
    suseconds_t time_start;
    suseconds_t time_left;
};

//GLOBAL STATIC VARIABLES   
struct rdp_pkt * window[WINDOW_SIZE];
extern int thread_state = 0;
pthread_mutex_t win_x;

//#############################################################################################
//UTILITY FUNCTIONS

//when a packet is sent must be added to list of unacked packets (window)
//"window" = list of unacked packets w timers, awaiting acking receipt
//creates rdp_pkt structure, adds to list
// undefined behaviour if window overflows
// window size defined as constant WINDOW_SIZE
int add_pkt_window(struct udp_pcb *pcb, struct pbuf *p){
    //create the packet 
    struct rdp_pkt * packet = create_pkt(pcb, p);

    pthread_mutex_lock(&win_x);
    //THIS DESIGN WILL NOT ADD TO WINDOW IF WINDOW IS FULL! BUT WILL STILL GET SEND 
    for(int i=0; i<WINDOW_SIZE; i++){
        if(window[i] == 0){ //entry is empty
            window[i] = packet;

            print_window();
            pthread_mutex_unlock(&win_x);

            return ERR_OK;
        }
    }
    pthread_mutex_unlock(&win_x);
    return ERR_NOTOK;
}

//upon receipt of an ack, delete_pkt is called to remove an entry a packet
//from the window of packets awaiting an ack
//delete packet by freeing memory and reiniting to 0 so we know it's empty
//if del_pkt called there should be a packet awaiting an ack
//if no packet found, returns an error
//note mutex lock encompasses pkt_match
int del_pkt_window(struct udp_pcb *pcb, struct pbuf *p){   
    pthread_mutex_lock(&win_x);  
    for(int i=0; i<WINDOW_SIZE; i++){
        if(window[i] != 0){            
            if(pkt_match(pcb, p, window[i]) == 1){        
                //free(window[i]->p->payload);
                //free(window[i]->p);
                //free(window[i]); 
                window[i] = 0; 
                pthread_mutex_unlock(&win_x);
                return ERR_OK;
            }
        }
    }
    pthread_mutex_unlock(&win_x);
    return ERR_NOTOK;
}

//#############################################################################################
//TIMER FUNCTIONS (for threads)

//main function to manage timers using a thread created in cli.c
//while the global var is set to not shut down
//thread updates timers in timer_update
//if there are expired timers in window, thread resends and restarts timer
void thread_timer_actions(){
   
    while(thread_state == 1){
        timer_update();
        timeout_event_handler();
        sleep(THREAD_SLEEP_TIME);
    }

    printf("Thread shutdown in process. \n");
    //free the remaining memory at shutdown
    for(int i=0; i<WINDOW_SIZE; i++){
        if(window[i] != 0){
                free(window[i]->p->payload);
                free(window[i]->p);
                free(window[i]); 
        }
    }
    return;
}

//function updates all rdp_pkt structs in window
//updates timer values based on time
//executed by thread 
void timer_update(){
    struct timeval clock;
    suseconds_t time_elapsed;
    suseconds_t time_rem;

    pthread_mutex_lock(&win_x);
    for(int i=0; i<WINDOW_SIZE; i++){
        if(window[i] != 0){ 

            gettimeofday(&clock, NULL);
            time_elapsed = ((clock.tv_sec * 1000) + (clock.tv_usec / 1000)) - window[i]->time_start;          
            window[i]->time_left = window[i]->time_left - time_elapsed;
        }
    }
    pthread_mutex_unlock(&win_x);
}

//handler for time_out events
//a function that checks for timed-out packets in the window
//if a packet timer is out (indicated in time_left of the rdp_pkt struct)
//calls udp_send using info from rdp_pkt in window
//also restarts the timer
void timeout_event_handler(){
    struct timeval clock;

    pthread_mutex_lock(&win_x);
    for(int i=0; i<WINDOW_SIZE; i++){
        if(window[i] != 0 && window[i]->time_left <= 0){ 

            //RESEND, create new pbuf packet and send
            struct pbuf *Xp = pbuf_alloc(PBUF_TRANSPORT, strlen(window[i]->p->payload), PBUF_RAM);
            Xp->payload = window[i]->p->payload;
            
            err_t e1 = udp_send(window[i]->pcb, Xp);  
                    if (e1 != ERR_OK)
                        printf("udp send error: %d\n", e1);
           
            //restart timers     
            gettimeofday(&clock, NULL);

            window[i]->time_start = ((clock.tv_sec * 1000) + (clock.tv_usec / 1000));
            window[i]->time_left = TIMEOUT_MS;           
            printf("The timed-out packet for %s was resent, timer restarted.\n", window[i]->p->payload);
        }
    }
    pthread_mutex_unlock(&win_x);
}


//#############################################################################################
//HELPER FUNCTIONS

//creates a rdp_pkt struct by copying from current pbuf
//this struct is entry placed in window for ack waiting
//returns a pointer to it
//ensured to deep copy payload so ctime_leftan match later
//memory allocated here freed in del_pkt
struct rdp_pkt * create_pkt(struct udp_pcb *pcb, struct pbuf *p){
    pthread_mutex_lock(&win_x);
    struct rdp_pkt * packet =(struct rdp_pkt *)malloc(sizeof(struct rdp_pkt));
    packet->pcb=pcb;

    //copy pbuf - NEED DEEP COPY        
    struct pbuf * p_copy = (struct pbuf *)malloc(sizeof(struct pbuf));
    *p_copy = *p;    
    char * payload_copy = malloc((sizeof(char))*DEFAULT_MTU);
    memcpy(payload_copy, p->payload, (sizeof(char))*DEFAULT_MTU);
    p_copy->payload = payload_copy;
    packet->p = p_copy;
   
    //set packet timer
    struct timeval clock;
    gettimeofday(&clock, NULL);
    packet->time_start = ((clock.tv_sec * 1000) + (clock.tv_usec / 1000));
    packet->time_left = TIMEOUT_MS;

    pthread_mutex_unlock(&win_x);

    return packet;
}

//helper function to find a matching packet in window
//check if rdp_pkt is a match based on payload
//logic is that if you ack the wrong packet bc same payload doesnt really matter
int pkt_match(struct udp_pcb *pcb, struct pbuf *p, struct rdp_pkt * packet){
    if(strcmp(p->payload,packet->p->payload) == 0){
        return 1;
    }
    else{
        return 0;
    }
}

//helper function to print window for debug
void print_window(){
    printf("----CURRENT WINDOW CONTENTS:----\n");
    for(int i=0; i<WINDOW_SIZE; i++){
        printf("----------------------------\n");
        if(window[i]!= 0 && window[i]->p->payload != NULL){
            printf(window[i]->p->payload);
            printf("\nPacket time left %d\n", window[i]->time_left);
       }
    } 
    printf("----------------------------\n");
}