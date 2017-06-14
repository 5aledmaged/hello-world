#ifndef	_USART_H_
#define	_USART_H_ 	1

#include<avr/io.h>
#include<util/delay.h>

/*The function is declared to initialize the USART with following cinfiguration:-
USART mode - Asynchronous
Baud rate - 9600
Data bits - 8
Stop bit - 1
Parity - No parity.*/

void usart_init();
/*The function is declared to transmit data.*/
void usart_data_transmit(unsigned char data );
/*The function is declared to receive data.*/
unsigned char usart_data_receive( void );
/*The function is declared to transmit string.*/
void usart_string_transmit(char *string);
/*The function is declared to receive string.*/
char *usart_string_receive(char *receive_string,unsigned char terminating_character);
/*Function defination*/
void usart_init()
{
UBRRH = 0;
//UBRRL =77; //Fosc=12MHz BR=9600 ;
//UBRRL =103; //Fosc=16MHz BR=9600 ;
//UBRRL =18; //Fosc=12MHz BR=38400 ;
//UBRRL =12; //Fosc=12MHz BR=57600 ;
UBRRL =16; //Fosc=12MHz BR=57600 ;

UCSRB|= (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
UCSRC |= (1 << URSEL)|(3<<UCSZ0);
//sei();
}


void usart_data_transmit(unsigned char data )
{
while ( !( UCSRA & (1<<UDRE)) );
UDR = data;
_delay_ms(1);
}

unsigned char usart_data_receive( void )
{
while ( !(UCSRA & (1<<RXC)) );
return UDR;
}

void usart_string_transmit(char *string)
{
while(*string)
{
usart_data_transmit(*string++);
}
}

char *usart_string_receive(char *receive_string,unsigned char terminating_character)
{
	unsigned char temp=0x00;
	unsigned char i=0;
	for(i=0;;i++)
	{
		*(receive_string+i)=usart_data_receive();
		if(*(receive_string+i)==terminating_character)
			break;
		else
		temp++;
	}
	*(receive_string+temp)='\0';
	return receive_string;
}
#endif
