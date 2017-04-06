 /**
 * @file   usart.h
 * @brief  
 * @date   2017/03/26 17:34:56
 * @author Jackhammer
 */ 
 
/*
 USART�ɂ���:
  * UBRR0 = ������g�� / (�{�[���[�g * 16) - 1
	�{�[���[�g : 2400
	������g�� : 1M
	�Ȃ��
	UBRR0 = 25
  * UCSR0B : ��M�E���M�L���ɂ���ݒ�
	000  1    �@�@1       000
	��M�L���@���M�L��
  * UCSR0C : �񓯊�����A�f�[�^�r�b�g����8bit�Ƃ��Đݒ�
	00000110
  * UDR0   : ����M����f�[�^�̃��W�X�^
  * UCSR0A��UDRE : ���M����OK�Ȃ�1�ɂȂ�
	����OK -> 00100000
  * UCSR0A��RXC  : ��M����OK�Ȃ�1�ɂȂ�
	��M -> 10000000
 see:
 https://plaza.rakuten.co.jp/ecircuit30/diary/200810180000/
 ������blog USART http://usicolog.nomaki.jp/engineering.html
 �n�߂�d�q��H(AVR) http://startelc.com/AVR/Avr_16trsUSB232c.html
 UART��USART�̈Ⴂ http://wa3.i-3-i.info/diff193uart.html
 https://www.appelsiini.net/2011/simple-usart-with-avr-libc
*/ 

#ifndef USART_H_
#define USART_H_

#include "config.h"

#define _USART_USE_CHARACTER_FUNCTION

#include <stdint.h>				// types
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>		// _BV��
#include <avr/interrupt.h>		// ����������


#if 0

#define USART_BUFF_SIZE_BIT_WIDTH		(5)
#define USART_BUFF_SIZE					(1<<USART_BUFF_SIZE_BIT_WIDTH)
#define USART_BUFF_SIZE_MASK			(USART_BUFF_SIZE-1)

#define INC_LOOP(x,mask)						x=(x+1)&(mask)

typedef struct
{
	char buf[24];
	volatile uint8_t seek_read;
			 uint8_t seek_write;
} ringbuffer;

ringbuffer	usart_recv;
ringbuffer	usart_send;


// ���荞�݂ɂ���M

static inline char usart_is_recieved()
{
	return (usart_recv.seek_read != usart_recv.seek_write);
}

static inline void usart_wait_for_recv()
{
	while(!usart_is_recieved());
}

static inline uint8_t usart_recieve_ring()
{
	usart_wait_for_recv();
	uint8_t seek = usart_recv.seek_read;
	cli();
	INC_LOOP(usart_recv.seek_read, USART_BUFF_SIZE_MASK);
	sei();
	return usart_recv.buf[seek];
}

// ���M�o�b�t�@�Ƀf�[�^������΁A��������1�o�C�g���M���郋�[�`���B
// �����I�Ɏg�p���Ă��邾���Ȃ̂Ń��[�U�[�͌Ăяo���Ȃ��ŁB
static inline void usart_private_send_char()
{
	// �l�������ꍇ�͑��M�o�b�t�@�͋�ł���
	if(usart_send.seek_read == usart_send.seek_write)
	{
		return;
	}

	uint8_t seek = usart_send.seek_read;
	cli();
	INC_LOOP(usart_send.seek_read, USART_BUFF_SIZE_MASK);
	sei();
	UDR = usart_send.buf[seek];// ���M�o�b�t�@�̃f�[�^�𑗐M(���̌㑗�M���荞�݂������B�o�b�t�@����ɂȂ�܂ő��M�������A�͂��B)
}



// 1�o�C�g���M
static inline void usart_send_char_ring(uint8_t data)
{
	// ���M�o�b�t�@�������ς��Ȃ�҂�
	while(((usart_send.seek_write + 1) & USART_BUFF_SIZE_MASK) == usart_send.seek_read);
	
	// ���͂Ƃ����ꑗ�M�o�b�t�@�Ƀf�[�^��ςށB
	usart_send.buf[usart_send.seek_write] = data;
	cli();
	INC_LOOP(usart_send.seek_write, USART_BUFF_SIZE_MASK);
	sei();

	// ���M���W�X�^���Z�b�g����Ă��� == ���M�ł����ԁ@�Ȃ�΁A
	// ��x�������M���Ă����B
	if(bit_is_set(UCSRA, UDRIE))
	{
		usart_private_send_char();
	}

	// �Ⴆ�Ύ��̂悤�ɑ��M�o�b�t�@�Ƀf�[�^��ς܂���UDR0�ɒ��ڃA�N�Z�X����R�[�h��
	// �悭�Ȃ��B
	// if (UCSR0A & (1<<UDRE0))
	//    UDR0 = c;
	// else
	//    usart_sendData[usart_send_write++] = c;
	// ����́Aelse�傪���s�����u�Ԃ�USART_TX_vect�ɂ�銄�荞�݂�������A
	// usart_send_write == usart_send_read�ł������ꍇ�A����sendChar���Ăяo�����
	// ���̑��M����������܂ł����Őς񂾃f�[�^�����M����Ȃ�����ł���B
}




#endif


//=============================================================================
/**
 * �V���A���ʐM�ɂ�1byte��M����
 * @brief	�V���A���ʐM�ɂ�1byte��M����
 * @return	1byte
 */
static inline uint8_t usart_recieve( void )
{
	loop_until_bit_is_set(UCSRA, RXCIE);		// ��M�����҂�:��M�����������UCSRA��RXCIE(7bit)��1�ɂȂ�
	
	return UDR;
}


//=============================================================================
/**
 * �V���A���ʐM�ɂ�1byte���M����
 * @brief	�V���A���ʐM�ɂ�1byte���M����
 * @param	���M�f�[�^
 * @return	none
 */
static inline void usart_transmit( uint8_t data )
{
	loop_until_bit_is_set(UCSRA, UDRIE);		// �o�b�t�@�ҋ@�҂�:���M���������o�b�t�@����ɂȂ��UCSRA��UDRIE(5bit)��1�ɂȂ�
	
	UDR = data;
}


//=============================================================================
/**
 * �V���A���ʐM�ɂđ��M����������܂őҋ@����B�����I�ȏ������K�v�ȍۗ��p����B
 * @brief	�V���A���ʐM�ɂđ��M����������܂őҋ@����
 * @return	none
 */
static inline void usart_flush( void )
{
	loop_until_bit_is_set(UCSRA, TXCIE);		// ���M�����҂�:���M�����������UCSRA��TXCIE(6bit)��1�ɂȂ�
}



void usart_recieve_bytes(char *pdata, unsigned char size);
void usart_transmit_bytes(const void* pdata, unsigned char size);

void usart_init();

#if defined _USART_USE_CHARACTER_FUNCTION
char usart_getc();
void usart_putc(char);

char* usart_gets(char *buf, unsigned char n);
void  usart_puts(const char *buf);


//int usart_getchar(FILE*);
//int usart_putchar(char, FILE*);

#endif // _USART_USE_CHARACTER_FUNCTION

#endif /* USART_H_ */
