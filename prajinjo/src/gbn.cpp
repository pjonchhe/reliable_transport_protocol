#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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

#define BUF_SIZE 1000
int nextSeqNum_A, maxSeqNum_A;
int expectedSeqNum_B, maxSeqNum_B;
int lastACKNum_A;
struct node
{
	struct node* next;
	struct pkt pktBuf;
};
struct node *head, *tail;
struct node *winHead, *winTail;
int numUnACK;
/*struct msg buffer[BUF_SIZE];
int front, rear, nextPkt;*/
float timeOut = 20.0;
struct pkt sndPkt_B;
void A_make_send_pkt();

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	/*if((front == 0 & rear = BUF_SIZE - 1) || (rear == front -1))
	{
		printf("Buffer FULL!!\n");
		return;
	}
	else if(front == -1)
	{
		front = 0;
		rear = 0;
		memcpy(buffer[rear].data, message.data, sizeof(message.data));
	}
	else
	{
		rear ++;
		memcpy(buffer[rear].data, message.data, sizeof(message.data))
	}
	if(front < rear)
	{
		if(nextPkt)
	}*/
	struct node* tempMsg = (struct node*)malloc(sizeof(struct node));
	memset(tempMsg->pktBuf.payload, 0, 20);
	memcpy(tempMsg->pktBuf.payload, message.data, sizeof(message.data));
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

	printf("Buffer message : %c\n", tempMsg->pktBuf.payload[0]);
	if(head == tail)
	{
		winHead = head;
		winTail = tail;
		//A_make_send_pkt();
	}
	A_make_send_pkt();
}

void A_make_send_pkt()
{
	int sum = 0;
	if(NULL == head)
		return;
	printf("numUnACK = %d\n", numUnACK);
	struct node* tempBuf = winHead;
	while(numUnACK < getwinsize())
	{
		if(numUnACK == 0)
			winTail = winHead;

		else if(winTail->next != NULL)
			winTail = winTail->next;

		else if(winTail->next == NULL)
			return;
		winTail->pktBuf.seqnum = nextSeqNum_A;
		winTail->pktBuf.acknum = 0;
		if(nextSeqNum_A == maxSeqNum_A)
			nextSeqNum_A = 0;
		else
			nextSeqNum_A ++;
		sum = winTail->pktBuf.seqnum + winTail->pktBuf.acknum;
		for(int i = 0; i < sizeof(winTail->pktBuf.payload); i++)
			sum = sum + (int)(winTail->pktBuf.payload[i]);

		winTail->pktBuf.checksum = sum;

		printf("sending : %c\n", winTail->pktBuf.payload[0]);
		tolayer3(0, winTail->pktBuf);
		if(winHead == winTail)
			starttimer(0, timeOut);

		numUnACK ++;

		if(winTail->next == NULL)
			break;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	int sum = 0;
	bool corrupt = true;
	sum = packet.seqnum + packet.acknum;
	printf("A_input Final sum is %d\n", sum);
	if(sum == packet.checksum)
		corrupt = false;
	else
		printf("A_input packet corrupt!!\n");
	
	if(corrupt)
		return;

	if(!corrupt)
	{
		printf("Received ACK for %d\n", packet.acknum);
		if(lastACKNum_A == packet.acknum)
		{
			printf("Duplicate ACK received!!\n");
			return;
		}
		lastACKNum_A = packet.acknum;
		struct node* tempNode = winHead;
		while(winHead->pktBuf.seqnum != packet.acknum)
		{
			winHead = winHead->next;
			free(tempNode);
			tempNode = winHead;
			head = winHead;
			numUnACK--;
		}

		//Remove last one also.
		if(winHead->pktBuf.seqnum == packet.acknum)
		{
			winHead = winHead->next;
			head = winHead;
			free(tempNode);
			numUnACK--;
		}
		if(numUnACK == 0)
			stoptimer(0);
		else
		{
			stoptimer(0);
			starttimer(0,timeOut);
		}
		A_make_send_pkt();
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("Timer Interrupt!!\n");
	starttimer(0, timeOut);

	struct node* tempNode = winHead;
	while(tempNode != winTail)
	{
		tolayer3(0, tempNode->pktBuf);
		tempNode = tempNode->next;
	}

	//Send the last one also.
	if(tempNode == winTail)
		tolayer3(0, tempNode->pktBuf);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	//base_A = 1;
	nextSeqNum_A = 0;
	maxSeqNum_A = getwinsize() + 1; 
	//front = -1;
	//rear = -1;
	numUnACK = 0;
	head = NULL;
	tail = NULL;
	winHead = NULL;
	winTail = NULL;
	lastACKNum_A = -1;
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
	printf("B_input Final sum is %d\n", sum);

	if(sum == packet.checksum)
		corrupt = false;
	else
		printf("B_input packet corrupt!!\n");

	if(!corrupt)
	{
		if(packet.seqnum == expectedSeqNum_B)
		{
			char tempMsg[20];
			memset(tempMsg, 0, 20);
			memcpy(tempMsg, packet.payload, sizeof(packet.payload));
			printf("tempMsg = %c\n", tempMsg[0]);
			tolayer5(1, tempMsg);
			sndPkt_B.seqnum = packet.seqnum;
			sndPkt_B.acknum = expectedSeqNum_B;
			sum = sndPkt_B.seqnum + sndPkt_B.acknum;
			printf("B_input ack = %d\n", sndPkt_B.acknum);
			sndPkt_B.checksum = sum;
			tolayer3(1, sndPkt_B);
			if(expectedSeqNum_B == maxSeqNum_B)
				expectedSeqNum_B = 0;
			else
				expectedSeqNum_B++;
		}
		else
		{
			printf("Sequence out of order received = %d expected = %d\n", packet.seqnum, expectedSeqNum_B);
			printf("Sending ack for seq = %d\n", sndPkt_B.seqnum);
			tolayer3(1, sndPkt_B);
		}
	}
	else
		tolayer3(1, sndPkt_B);
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	sndPkt_B.seqnum = 0;
	sndPkt_B.acknum = -1;
	sndPkt_B.checksum = 0;
	memset(sndPkt_B.payload, 0, 20);
	expectedSeqNum_B = 0;
	maxSeqNum_B = getwinsize() + 1; 
}
