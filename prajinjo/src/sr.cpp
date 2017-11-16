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

#define TIMEOUT_VAL	18
int nextSeqNum_A, maxSeqNum_A;
int expectedSeqNum_B, maxSeqNum_B;

struct node
{
	struct node* next;
	struct pkt pktBuf;
	int timeOut;
	bool bAckReceived;
};
struct node *head_A, *tail_A;
struct node *winHead_A, *winTail_A;
int numUnACK;

struct node *head_B, *tail_B;
int numBuf_B;
struct pkt sndPkt_B;

float timer = 1.0;

void A_make_send_pkt();

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
	struct node* tempMsg = (struct node*)malloc(sizeof(struct node));
	memset(tempMsg->pktBuf.payload, 0, 20);
	memcpy(tempMsg->pktBuf.payload, message.data, sizeof(message.data));
	tempMsg->bAckReceived = false;
	tempMsg->next = NULL;
	if(head_A == NULL)
	{
		head_A = tempMsg;
		tail_A = tempMsg;
	}
	else
	{
		tail_A->next = tempMsg;
		tail_A = tempMsg;
	}

	printf("Buffer message : %c\n", tempMsg->pktBuf.payload[0]);
	if(head_A == tail_A)
	{
		winHead_A = head_A;
		winTail_A = tail_A;
	}
	A_make_send_pkt();
}

void A_make_send_pkt()
{
	int sum = 0;
	if(NULL == head_A)
		return;

	printf("numUnACK = %d\n", numUnACK);
	struct node* tempBuf = winHead_A;

	if(numUnACK >= getwinsize())
		printf("Window FULL !!!\n");
	while(numUnACK < getwinsize())
	{
		if(numUnACK == 0)
			winTail_A = winHead_A;

		else if(winTail_A->next != NULL)
			winTail_A = winTail_A->next;

		else if(winTail_A->next == NULL)
			return;

		winTail_A->pktBuf.seqnum = nextSeqNum_A;
		winTail_A->pktBuf.acknum = 0;

		if(nextSeqNum_A == maxSeqNum_A)
			nextSeqNum_A = 0;
		else
			nextSeqNum_A ++;

		sum = winTail_A->pktBuf.seqnum + winTail_A->pktBuf.acknum;
		for(int i = 0; i < sizeof(winTail_A->pktBuf.payload); i++)
			sum = sum + (int)(winTail_A->pktBuf.payload[i]);

		winTail_A->pktBuf.checksum = sum;

		printf("sending : %c %d\n", winTail_A->pktBuf.payload[0], winTail_A->pktBuf.seqnum);
		tolayer3(0, winTail_A->pktBuf);

		winTail_A->timeOut = TIMEOUT_VAL;
		winTail_A->bAckReceived = false;
		if(numUnACK == 0)
			starttimer(0, timer);

		numUnACK ++;
		if(winTail_A->next == NULL)
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
		struct node* tempNode = winHead_A;
		if(winHead_A == NULL)
			return;
		while(tempNode && tempNode->pktBuf.seqnum != packet.acknum)
		{
			tempNode = tempNode->next;
		}

		if(tempNode == NULL)
		{
			// Already ACK received.
			return;
		}
		if(tempNode->pktBuf.seqnum == packet.acknum)
		{
			tempNode->bAckReceived = true;
			tempNode->timeOut = 0;
		}

		if(tempNode == winHead_A)
		{
			while(winHead_A && winHead_A->bAckReceived != false)
			{
				winHead_A = winHead_A->next;
				free(tempNode);
				tempNode = winHead_A;
				head_A = winHead_A;
				numUnACK--;
			}
		}
		if(numUnACK == 0)
			stoptimer(0);
		A_make_send_pkt();
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	struct node* tempNode = winHead_A;
	while(tempNode && tempNode != winTail_A)
	{
		tempNode->timeOut--;
		if(tempNode->timeOut == 0)
		{
			tolayer3(0, tempNode->pktBuf);
			printf("Sending again : %c %d\n", tempNode->pktBuf.payload[0], tempNode->pktBuf.seqnum);
			tempNode->timeOut = TIMEOUT_VAL;
		}
		tempNode = tempNode->next;
	}
	//Check the last one also.
	if(tempNode && tempNode == winTail_A)
	{
		tempNode->timeOut--;
		if(tempNode->timeOut == 0)
		{
			printf("Sending again : %c %d\n", tempNode->pktBuf.payload[0], tempNode->pktBuf.seqnum);
			tolayer3(0, tempNode->pktBuf);
			tempNode->timeOut = TIMEOUT_VAL;
		}
	}
	if(numUnACK != 0)
		starttimer(0, timer);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	nextSeqNum_A = 0;
	//maxSeqNum_A = getwinsize() * 2;
	maxSeqNum_A = 1000;
	numUnACK = 0;
	head_A = NULL;
	tail_A = NULL;
	winHead_A = NULL;
	winTail_A = NULL;
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
		sndPkt_B.seqnum = packet.seqnum;
		sndPkt_B.acknum = packet.seqnum;
		sum = sndPkt_B.seqnum + sndPkt_B.acknum;
		printf("B_input ack = %d\n", sndPkt_B.acknum);
		printf("packet.seqnum = %d expectedSeqNum_B = %d numBuf = %d\n",packet.seqnum, expectedSeqNum_B, numBuf_B);
		sndPkt_B.checksum = sum;
		tolayer3(1, sndPkt_B);

		if(packet.seqnum == expectedSeqNum_B)
		{
			char tempMsg[20];
			memset(tempMsg, 0, 20);
			memcpy(tempMsg, packet.payload, sizeof(packet.payload));
			printf("tempMsg = %c \n", tempMsg[0]);
			printf("Send aboce1 at b = %c\n", tempMsg[0]);
			tolayer5(1, tempMsg);
			if(expectedSeqNum_B == maxSeqNum_B)
			{
				expectedSeqNum_B = 0;
			}
			else
			{
				expectedSeqNum_B++;
			}
			if(numBuf_B != 0)
			{
				while(head_B && head_B->pktBuf.seqnum == expectedSeqNum_B)
				{
					struct node* tempNode = head_B;
					memset(tempMsg, 0, 20);
					memcpy(tempMsg, tempNode->pktBuf.payload, sizeof(tempNode->pktBuf.payload));
					printf("Send aboce2 at b = %c\n", tempMsg[0]);
					tolayer5(1, tempMsg);
					head_B = head_B->next;
					free(tempNode);
					numBuf_B --;
					if(expectedSeqNum_B == maxSeqNum_B)
						expectedSeqNum_B = 0;
					else
						expectedSeqNum_B++;
				}
			}
			printf("After sendig numBuf = %d\n", numBuf_B);
		}
		else 
		{
			if((((expectedSeqNum_B + getwinsize() -1) <= maxSeqNum_B) && (packet.seqnum > expectedSeqNum_B) && (packet.seqnum <= (expectedSeqNum_B + getwinsize() -1))) || 
				(((expectedSeqNum_B + getwinsize() -1) > maxSeqNum_B) && (((packet.seqnum > expectedSeqNum_B) && (packet.seqnum <= maxSeqNum_B)) || (packet.seqnum < (expectedSeqNum_B + getwinsize() -1 - maxSeqNum_B - 1)))))
			{

				//Need to check if already buffered.
				struct node* checkNode = head_B;
				bool bBuffered = false;
				while(checkNode)
				{
					if(checkNode->pktBuf.seqnum == packet.seqnum)
					{
						bBuffered = true;
						break;
					}
					checkNode = checkNode->next;
				}

				if(bBuffered)
					return;

				struct node* tempNode = (struct node*)malloc(sizeof(struct node));
				memset(tempNode->pktBuf.payload, 0, 20);
				memcpy(tempNode->pktBuf.payload, packet.payload, sizeof(packet.payload));
				tempNode->pktBuf.seqnum = packet.seqnum;
				tempNode->pktBuf.acknum = packet.acknum;
				tempNode->next = NULL;
				printf("buffer at B = %c\n", packet.payload[0]);
				if(head_B == NULL)
				{
					head_B = tempNode;
					tail_B = tempNode;
				}
				else
				{
					if(tempNode->pktBuf.seqnum < head_B->pktBuf.seqnum)
					{
						tempNode->next = head_B;
						head_B = tempNode;
					}
					else
					{
						struct node* traverse = head_B;
						while(traverse->next != NULL && traverse->next->pktBuf.seqnum < tempNode->pktBuf.seqnum)
							traverse = traverse->next;

						tempNode->next = traverse->next;
						traverse->next = tempNode;
						if(tempNode->next == NULL)
							tail_B = tempNode;
					}
				}
				numBuf_B ++;
			}
		}
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	head_B = NULL;
	tail_B = NULL;
	sndPkt_B.seqnum = 0;
	sndPkt_B.acknum = -1;
	sndPkt_B.checksum = 0;
	memset(sndPkt_B.payload, 0, 20);
	expectedSeqNum_B = 0;
	//maxSeqNum_B = getwinsize() * 2;
	maxSeqNum_B = 1000; 
}
