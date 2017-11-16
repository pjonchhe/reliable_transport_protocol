#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

enum states_A {waitForCall0, waitForAck0, waitForCall1, waitForAck1};
enum states_B {waitFor0, waitFor1};
int curState_A;
int curState_B;
float timeOut = 18.0;
struct pkt sndPkt_A, sndPkt_B;
struct node *head, *tail;
struct node
{
	struct node* next;
	char data[20];
};
void A_make_send_pkt();
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	struct node* tempMsg = (struct node*)malloc(sizeof(struct node));
	memcpy(tempMsg->data, message.data, sizeof(message.data));
	tempMsg->next = NULL;
	if(head == NULL)
	{
		head = tempMsg;
		tail = tempMsg;
	}
	else
	{
		tail->next = tempMsg;
		tail = tempMsg;
	}	
	
	printf("Enter into buffer: %c\n", message.data[0]);
	if((head == tail) && ((waitForCall0 == curState_A) || (waitForCall1 == curState_A)))
	{
		A_make_send_pkt();
	}
}

void A_make_send_pkt()
{
	if(NULL == head)
		return;
	int sum = 0;
	if(waitForCall0 == curState_A)
	{
		sndPkt_A.seqnum = 0;
		sndPkt_A.acknum = 0;
	}
	else if(waitForCall1 == curState_A)
	{
		sndPkt_A.seqnum = 1;
		sndPkt_A.acknum = 0;
	}
	printf("A_make_send_pkt sndPkt_A.seqnum = %d sndPkt_A.acknum = %d\n",sndPkt_A.seqnum, sndPkt_A.acknum);
	sum = sndPkt_A.seqnum + sndPkt_A.acknum;
	printf("A_make_send_pkt buffer[0].data = %c\n", head->data[0]);
	for(int i = 0; i < sizeof(head->data); i++)
 	{
		sum = sum + (int)(head->data[i]);
	}
	sndPkt_A.checksum = sum;
	printf("A_output Final check sum is %d\n", sndPkt_A.checksum);
	memset(sndPkt_A.payload, 0, 20);
	memcpy(sndPkt_A.payload, head->data, sizeof(head->data));
	struct node* temp = head;
	head = head->next;
	free(temp);
	tolayer3(0, sndPkt_A);
	starttimer(0, timeOut);
	if(waitForCall0 == curState_A)
	{
		curState_A = waitForAck0;
	}
	else if(waitForCall1 == curState_A)
	{
		curState_A = waitForAck1;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	int sum = 0;
	bool corrupt = true;
	sum = packet.seqnum + packet.acknum;
	//printf("A_input SUM of ack and seq = %d\n", sum);
	printf("A_input Final sum is %d\n", sum);
	if(sum == packet.checksum)
	{
		corrupt = false;
		//printf("A_input packet not corrupt!!\n");
	}
	else
		printf("A_input packet corrupt!!\n");
	
	if(corrupt)
		return;
	
	if(!corrupt)
	{
		if(waitForAck0 == curState_A)
		{
			if(0 == packet.acknum)
			{
				stoptimer(0);
				printf("Timer stopped for 0\n");
				curState_A = waitForCall1;
				A_make_send_pkt();
			}
			else
			{
				printf("A_input wrong ack 1!!\n");
				return;
			}
		}
		else if(waitForAck1 == curState_A)
		{
			if(1 == packet.acknum)
			{
				stoptimer(0);
				printf("Timer stopped for 1\n");
				curState_A = waitForCall0;
				A_make_send_pkt();
			}
			else
			{
				printf("A_input wrong ack 1!!\n");
				return;
			}
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("Timer Interrupt!!\n");
	tolayer3(0, sndPkt_A);
	starttimer(0, timeOut);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	curState_A = waitForCall0;
	sndPkt_A.seqnum = 0;
	sndPkt_A.acknum = 0;
	sndPkt_A.checksum = 0;
	memset(sndPkt_A.payload, 0, 20);
	head = NULL;
	tail = NULL;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
	int sum = 0;
	bool corrupt = true;
	int checksum;
	sum = packet.seqnum + packet.acknum;
	printf("B_input seq = %d checksum = %d\n", packet.seqnum, packet.checksum);
	for(int i = 0; i < sizeof(packet.payload); i++)
	{
		sum = sum + (int)packet.payload[i];
	}
	//sum = sum + packet.checksum;
	printf("B_input Final sum is %d\n", sum);
	//checksum = ~sum;
	if(sum == packet.checksum)
	{
		corrupt = false;
		//printf("B_input packet not corrupt!!\n");
	}
	else
		printf("B_input packet corrupt!!\n");
	if(corrupt)
	{
		tolayer3(1, sndPkt_B);
		return;
	}
	
	if(waitFor0 == curState_B)
	{
		if(0 == packet.seqnum)
		{
			char tempMsg[20];
			memset(tempMsg, 0, 20);
			memcpy(tempMsg, packet.payload, sizeof(packet.payload));
			tolayer5(1, tempMsg);
			sndPkt_B.seqnum = 0;
			sndPkt_B.acknum = 0;
			sum = sndPkt_B.seqnum + sndPkt_B.acknum;
			printf("B_input ack = %d\n", sndPkt_B.acknum);
			sndPkt_B.checksum = sum;
			//printf("B_input Final check sum is %d\n", sndPkt_B.checksum);
			tolayer3(1, sndPkt_B);
			curState_B = waitFor1;
		}
		else
			tolayer3(1, sndPkt_B);
	}
	else if(waitFor1 == curState_B)
	{
		if(1 == packet.seqnum)
		{
			char tempMsg[20];
			memset(tempMsg, 0, 20);
			memcpy(tempMsg, packet.payload, sizeof(packet.payload));
			tolayer5(1, tempMsg);
			sndPkt_B.seqnum = 1;
			sndPkt_B.acknum = 1;
			sum = sndPkt_B.seqnum + sndPkt_B.acknum;
			printf("B_input ack = %d\n", sndPkt_B.acknum);
			sndPkt_B.checksum = sum;
			//printf("B_input Final check sum is %d\n", sndPkt_B.checksum);
			tolayer3(1, sndPkt_B);
			curState_B = waitFor0;
		}
		else
			tolayer3(1, sndPkt_B);
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	curState_B = waitFor0;
	sndPkt_B.seqnum = 0;
	sndPkt_B.acknum = -1;
	sndPkt_B.checksum = 0;
	memset(sndPkt_B.payload, 0, 20);
}
