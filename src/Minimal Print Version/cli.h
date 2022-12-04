/*
 * cli.h (header file for the Command line handler)
 *
 * AUTHOR: Muthucumaru Maheswaran
 * DATE:   December 25, 2004
 *
 * The CLI is used as a configuration file parser
 * as well. Right now the CLI module is only capable
 * of parsing a very simple format and limited command set...
 * Future development should make the CLI more IOS like??
 */

#ifndef __CLI_H__
#define __CLI_H__

#include <string.h>
#include <stdio.h>
#include <slack/std.h>
#include <slack/map.h>
#include "message.h"
#include "grouter.h"


#define PROGRAM                       10
#define JOIN                          11
#define COMMENT                       12
#define COMMENT_CHAR                  '#'
#define CONTINUE_CHAR                 '\\'
#define CARRIAGE_RETURN               '\r'
#define LINE_FEED                     '\n'

#define MAX_BUF_LEN                   4096
#define MAX_LINE_LEN                  1024
#define MAX_KEY_LEN                   64



typedef struct _cli_entry_t
{
	char keystr[MAX_KEY_LEN];
	char long_helpstr[MAX_BUF_LEN];
	char short_helpstr[MAX_BUF_LEN];
	char usagestr[MAX_BUF_LEN];
	void (*handler)();
} cli_entry_t;


// function prototypes...
void dummyFunction();
void parseACLICmd(char *str);
void CLIProcessCmds(FILE *fp, int online);
void CLIPrintHelpPreamble();
void *CLIProcessCmdsInteractive(void *arg);
void registerCLI(char *key, void (*handler)(),
		 char *shelp, char *usage, char *lhelp);
void CLIDestroy();
int CLIInit(router_config *rarg);

//#############################################################################################
// wrapper for udp_send that also adds a packet to the window awaiting acknowledgement
// calls add_pkt_window and udp_send
// returns an int use for error check 
int rdp_send(struct udp_pcb *pcb , struct pbuf *p);

//callback function for receipt of a message
//determines whether a message received was ack or msg via src port value
//if ack, deletes pkt from window, if msg, sends and ack
//calls del_pkt_window, send_ack and udp functions  

void rdp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, uchar *addr, uint16_t port_raw);
//function called to send an ack upon receipt of a message
//construct a new pcb w special flags in the port #
//does this by temporarily modifying the src port
//resends original payload
//calls udp_send
void send_ack(struct udp_pcb *pcb, struct pbuf *p);
//#############################################################################################


void helpCmd();
void versionCmd();
void setCmd();
void getCmd();
void sourceCmd();
void ifconfigCmd();
void routeCmd();
void arpCmd();
void pingCmd();
void consoleCmd();
void haltCmd();
void queueCmd();
void qdiscCmd();
void spolicyCmd();
void classCmd();
void filterCmd();
void openflowCmd();
void gncCmd();
void gncTerminate();

#endif
