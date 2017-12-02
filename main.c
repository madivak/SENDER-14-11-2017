
/*
 * GPS-UART-BUF.c
 *
 * Created: 10/13/2017 5:37:46 PM
 * Author : madiv
 */ 
#define F_CPU 1000000UL 
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "sms.h"

#define FOSC 2000000UL // Clock Speed
#define BAUD 9600
#define MYUBRR (((FOSC / (BAUD * 16UL))) - 1)

static FILE uart0_output = FDEV_SETUP_STREAM(USART0_Transmit, NULL, _FDEV_SETUP_WRITE);
static FILE uart1_output = FDEV_SETUP_STREAM(USART1_Transmit, NULL, _FDEV_SETUP_WRITE);
static FILE uart1_input = FDEV_SETUP_STREAM(NULL, USART1_Receive, _FDEV_SETUP_READ);
static FILE uart0_input = FDEV_SETUP_STREAM(NULL, USART0_Receive, _FDEV_SETUP_READ);

int CheckSMS();
void checkOKstatus();
char sample_GPS_data (void);
int CarStatus(int p);
int sender();
int CompareNumber();
int initialstatus();
int PrintSender();

#define CAR_ON	PORTB |= (1<<PORTB1)
#define CAR_OFF	PORTB &= ~(1<<PORTB1)

int a; int i; char w; int y;
char input;
char buff[20];
char company[]	= "+2547xxxxxxxx"; //moha's No#
char company2[]	= "+2547xxxxxxxx"; //fatah's no#
char owner[]	= "+2547xxxxxxxx"; //kevin's no#
//char owner[]	= "+2547xxxxxxxx"; //danstan's no#

int main( void )
{
	CAR_OFF;
	
	USART1_Init(MYUBRR);
	USART0_Init(MYUBRR);
	DDRB |= (1<<DDB1); //set PORTB1 as output
	//Interaction between ATMEGA & GSM	
 	stdin = &uart0_input;
 	stdout = &uart0_output;
//	sei();
	
	
	_delay_ms(13000);
	initialstatus();
	while(1) 
	{
		//check availability of SMS
		int n = 0;
		CheckSMS(); //check if available unread SMS and its content
		int f = buff[13]; // get car status value from buff[13
		
		//Alter status of car
 		int y = buff[14] + buff[15] + buff[16]; //sum values of the 3 buffer values		
/////////////////////////////////////////////////////////////////////////////////////////////////////////////		
 		int y = buff[14] + buff[15] + buff[16]; //sum values of the 3 buffer values
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//Interaction between GPS,ATMEGA & GSM
		fdev_close();
		stdout = &uart0_output;
		stdin = &uart1_input;
		sample_GPS_data ();
/////////////////////////////////////////////////////////////////////////////////////////////////////////////		
		fdev_close(); // monitor proceedings
		stdout = &uart1_output;
		printf("\r\nPrinting buffer");
		printf("\r\nCar status is = %d\r\nText no# is :", f);
		while (n < 13)
		{
			printf("%c", buff[n]);
			n++;
		}
		printf("\r\nbuff[14] = %d", buff[14]);
		printf("\r\nbuff[15] = %d", buff[15]);
		printf("\r\nbuff[16] = %d", buff[16]);
		printf("\r\nbuff[14] + buff[15] + buff[16] = %d", y);
		printf("\r\nbuffer end\r\n");
/////////////////////////////////////////////////////////////////////////////////////////////////////////////		
		fdev_close();
		stdout = &uart0_output;
		stdin = &uart0_input;
		_delay_ms(4000);
	}
	return 0;
}

int CheckSMS()
{
	int z = 0; //char w;
	y=0;
	a=0;
	printf("AT\r\n");
	checkOKstatus();
	
	printf("AT+CMGF=1\r\n");
	checkOKstatus();
	
	printf("AT+CMGL=\"REC UNREAD\"\r\n");
	while (a < 2) //skip the <LF>
	{
		w = getchar();
		if (w==0x0A)
		{ a++; }
		else
		{}
	}
	w = getchar();
	
	if (w==0x02B) // if w = +
	{
		sender();
		w = getchar();
		while (w !=  0x0A) //w is not <LF>
		{ w = getchar();}
		
		w = getchar();
		if (w == 0x030)//w is '0'
		{	
			CompareNumber();
			z = buff[14] + buff[15] + buff[16]; //sum values of the 3 buffer values
			if (z < 3) //A scenario of receiving text from an authorized no# with '0'
				{	buff[13] = 1;
					CAR_OFF; 
				}
			else //A scenario of receiving text from Unauthorized no#
			{buff[13] = 0; }
			  
		}
		else if(w == 0x031)//w is '1'
		{ 
			CompareNumber();
			z = buff[14] + buff[15] + buff[16]; //sum values of the 3 buffer values
			if (z < 3) //A scenario of receiving text from an authorized no# with '1'
			{	buff[13] = 2;
				CAR_ON;
			}
			else //A scenario of receiving text from Unauthorized no#
			{buff[13] = 0; }  
		}
		else{buff[13] = 6; buff[14] = buff[15] = buff[16] = 0; }
	}
	else if(w==0x04F) // if w = 'O'
	{
		w = getchar();
		if (w==0x04B) // if w = 'K', if there is no new sms
		{	buff[13] = 3; buff[14] = buff[15] = buff[16] = 0; 
			initialstatus();
		}
		else
		{ buff[13] = 4; buff[14] = buff[15] = buff[16] = 0; }
		
	}
	else
	{buff[13] = 5; buff[14] = buff[15] = buff[16] = 0; }
		
	int E = buff[13];
		if (E==1) // clear sms storage area if 0/1 is received
		{
//			printf("AT+CMGF=1\r\n");
//			checkOKstatus();
			printf("AT+CMGD=1,4\r\n"); //clearing all SMS in storage AREA
			checkOKstatus();
			printf("AT+CMGW=\"");
			PrintSender();
			printf("\",145,\"STO UNSENT\"\r\n");
			_delay_ms(2000);
			printf("0");
			putchar(0x1A); //putting AT-MSG termination CTRL+Z in USART0
		}
		else if (E==2) // clear sms storage area if 0/1 is received
		{
//			printf("AT+CMGF=1\r\n");
//			checkOKstatus();
			printf("AT+CMGD=1,4\r\n"); //clearing all SMS in storage AREA
			checkOKstatus();
			printf("AT+CMGW=\"");
			PrintSender();
			printf("\",145,\"STO UNSENT\"\r\n");
			_delay_ms(2000);
			printf("1");
			putchar(0x1A); //putting AT-MSG termination CTRL+Z in USART0
		}
		else
		{}

	return *buff;
}

void checkOKstatus()
{
		w = getchar();
		while (w!=0x04F) // if w = O
		{	w = getchar();	}
			
		while (w!=0x04B) // if w = K
		{	w = getchar();	}
}

int CarStatus(int p)
{
	int c;
	c = p;
	if (c == 1)
	{ CAR_OFF; }
	else if (c == 2)
	{ CAR_ON; }
	else
	{}
	return 0;
}

int sender()
{
	int n;
	w = getchar();
	while (w != 0x02B) // while w is not +
	{ w = getchar(); }
	
	for (n=0; n<13; n++) //capture 13 digit phone number
	{	buff[n] = w;
	w = getchar();}
	
	return *buff;
}

int PrintSender()
{
	int n;
	for (n=1; n<13; n++) //capture 13 digit phone number
	{	printf("%c", buff[n]);}
	return 0;
}

int CompareNumber()
{
	int j;
	buff[14]=buff[15]=buff[16]=0;
	
	for (j=0; j<13; j++)
	{
		if (buff[j]!=company[j])
		{ buff[14] = 1;}
		else{}
		if (buff[j]!=owner[j])
		{ buff[15] = 1;}
		else{}
		if (buff[j]!=company2[j])
		{ buff[16] = 1;}
		else{}
	}
	return *buff;
}

char sample_GPS_data (void)
{
	int i=0;
	printf("AT\r\n");
	_delay_ms(1000);
	printf("AT+CGATT=1\r\n");
	_delay_ms(2000);
	printf("AT+CIPMUX=0\r\n");
	_delay_ms(1000);
	printf("AT+CSTT=\"APN\",\"\",\"\"\r\n");
	_delay_ms(2000);
	printf("AT+CIICR\r\n");
	_delay_ms(3000);
	printf("AT+CIFSR\r\n");
	_delay_ms(2000);
	printf("AT+CIPSTART=\"TCP\",\"SERVER IP\",\"PORT\"\r\n");
	_delay_ms(1000);
	printf("AT+CIPSEND\r\n");
	_delay_ms(2000);
	printf("\r\nCAR PLATE NO:[KBY-xxxx] \r\n$GPGG");
	while(i == 0)
	{
		input = getchar();
		if (input == 0x024) //if the character is "$"
		{
			input = getchar();
			if (input == 0x047) //if the character is "G"
			{
				input = getchar();
				if (input == 0x050) //if the character is "P"
				{
					input = getchar();
					if (input != 0x047) //if the character is not "G"
					{}
					else
					{
						input = getchar();
						if (input != 0x047) //if the character is not "G"
						{}
						else
						{
							input = getchar();
							while (input != 0x024)  //if the character is not "$"
							{
								putchar(input); //Get GPGGA data
								input = getchar();
							}	
							i=1;
							
						}
					}
				}
				else{}
			}
			else{}
		}
		else{}
	}
	
	printf("$GPRM");
	while(i == 1)
		{
			input = getchar();
			if (input == 0x024) //if the character is "$"
			{
				input = getchar();
				if (input == 0x047) //if the character is "G"
				{
					input = getchar();
					if (input == 0x050) //if the character is "P"
					{
						input = getchar();
						if (input != 0x052) //if the character is not "R"
						{}
						else
						{
							input = getchar();
							if (input != 0x04D) //if the character is not "M"
							{}
							else
							{
								input = getchar();
								while (input != 0x024)  //if the character is not "$"
								{
									putchar(input); //Get GPGGA data
									input = getchar();
								}
								i=2;
							}
						}
					}
					else{}
				}
				else{}
			}
			else{}
		}
	
	printf("BUFF[13] = %d\r\n", buff[13]);
	_delay_ms(200);
	printf("Car status is = %d\r\nText no# is :", buff[13]);
	PrintSender();
	printf("\r\n");
	putchar(0x1A); //putting AT-MSG termination CTRL+Z in USART0
	_delay_ms(2000);
	printf("AT+CIPCLOSE\r\n");
	_delay_ms(2000);
	return 0;
}

int initialstatus()
{
	//char w;
	y=0;
	a=0;
	printf("AT\r\n");
	_delay_ms(2000);
	
	printf("AT+CMGF=1\r\n");
	_delay_ms(2000);
	
	printf("AT+CPMS=\"MT\",\"SM\",\"ME\"\r\n");
	_delay_ms(2000);
	
	printf("AT+CMGR=1\r\n");
	while (a < 2) //skip the <LF>
	{
		w = getchar();
		if (w==0x0A)
		{ a++; }
		else
		{}
	}
	w = getchar();
	
	if (w==0x02B) // if w = +
	{
		sender();
		
		w = getchar();
		while (w !=  0x0A) //w is not <LF>
		{ w = getchar();}
		
		w = getchar();
		if (w == 0x030)//w is '0'
		{ buff[13] = 1; _delay_ms(200); CAR_OFF; }
		else if(w == 0x031)//w is '1'
		{ buff[13] = 2; _delay_ms(200); CAR_ON;  }
		else{buff[13] = 6;}
	}
	
	else
	{}

	return *buff;
}
