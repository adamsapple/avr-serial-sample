/**
 * @file   usart.c
 * @brief  
 * @date   2017/03/26 17:35:08
 * @author Jackhammer
 */ 

#include "usart.h"

#include <stdint.h>				// types
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/sfr_defs.h>		// _BV��
#include <util/setbaud.h>		// F_CPU�ABAUD�錾�̃`�F�b�N

//! �V���A������̓��̓o�b�t�@
char *stdin_buffer;//[16];
//! �G�R�[�o�͂���ꍇ��1
char is_echo = 1;


#if 0

// ���荞�݂ɂ�鑗�M
// ���M�V�t�g���W�X�^����ɂȂ�A1�t���[���̑��M���I��������ɔ���
ISR(USART_TX_vect)
{
	usart_private_send_char();
}

// �����݂ɂ�鑗�M
ISR(USART_RX_vect)
{
	usart_recv.buf[usart_recv.seek_write] = UDR;    // ��M�f�[�^����M�o�b�t�@�Ɋi�[
	cli();
	INC_LOOP(usart_recv.seek_write, USART_BUFF_SIZE_MASK);
	sei();
}

// UDR���󂢂��Ƃ��ɔ���
ISR(USART_UDRE_vect)
{
	usart_private_send_char();
}
#endif

void set_stdin(char *ptr){
	stdin_buffer = ptr;
}

//=============================================================================
/**
 * �V���A������o�C�i���f�[�^����擾����
 * @brief	�V���A������o�C�i���f�[�^���擾����
 * @param	(pdata) �f�[�^�i�[��
 * @param	(size) �o�̓T�C�Y(bytes)
 * @return	size
 */
 void usart_recieve_bytes(char *pdata, unsigned char size)
 {
	memset(pdata, 0, size);

	while(size-->0)
	{
		*pdata = usart_recieve();
		pdata++;
	}
 }

//=============================================================================
/**
 * �V���A���փo�C�i���f�[�^����o�͂���
 * @brief	�o�C�i���f�[�^���o�͂���
 * @param	(pdata) �o�͂���f�[�^
 * @param	(size) �o�̓T�C�Y(bytes)
 * @return	none
 */
 void usart_transmit_bytes(const void *pdata, unsigned char size)
{
	const char* p = pdata;
	while( size-->0 )
	{
		usart_transmit( *p );
		p++;
	}

	usart_flush();
}


//static FILE usart_output	= FDEV_SETUP_STREAM(usart_putchar, NULL, _FDEV_SETUP_WRITE);
//static FILE usart_input	= FDEV_SETUP_STREAM(NULL, usart_getchar, _FDEV_SETUP_READ);

//=============================================================================
/**
 * FCPU,BAUD�̐ݒ�l�����usart�̃{�[���[�g���̐ݒ���s���B
 * @brief	usart������
 * @return	none
 */
void usart_init()
{
	//stdout = &usart_output;
	//stdin  = &usart_input;

	UBRRH	= UBRRH_VALUE;				// (USART Baud Rate Register Hi)
	UBRRL	= UBRRL_VALUE;				// (USART Baud Rate Register Lo) ��2313�f�[�^�V�[�g��P88,89���Q��
}

#if defined _USART_USE_CHARACTER_FUNCTION

//=============================================================================
/**
 * �V���A������1�����擾����
 * @brief	�V���A�����͂�1�����擾����
 * @return	1����
 */
char usart_getc()
{
	return usart_recieve();
}


//=============================================================================
/**
 * �V���A����1�����o�͂���
 * @brief	�V���A����1�����o�͂���
 * @param	(c) �o�͂��镶��
 * @return	none
 */
void usart_putc(char c)
{
	usart_transmit(c);
}


//=============================================================================
/**
 * �V���A���֕�������o�͂���
 * @brief	�V���A���֕�������o�͂���
 * @param	(pstr) �o�͂��镶����
 * @return	none
 */
 void usart_puts(const char* pstr)
{
	while( *pstr != '\0' )
	{
		usart_transmit( *pstr );
		pstr++;
	}

	usart_flush();
}


//=============================================================================
/**
 * �V���A�����͂𕶎���Ƃ��ĕԂ��B
 * ���s(\r\n)�͊܂܂Ȃ��Anull�I�[�t���B�ő�31+1�����B
 * @brief	�V���A�����͂𕶎���Ƃ��ĕԂ��B
 * @param	(buf) �ԋp�敶����
 * @param	(n) buf�̃T�C�Y
 * @return	none
 */
char* usart_gets(char *buf, unsigned char n)
{
	unsigned char c, i = 0;

	memset(buf, 0, n);


	for(i=0; i<n; ++i)
	{
		// �o�b�t�@�I�[����Ȃ���Γ��͑҂�
		// �I�[�Ȃ玩����null����
		if(i < n-1)
		{
			c = usart_recieve();
		}else{
			c = '\0';
		}
		
		// ���s�R�[�h��null��������
		if(c == '\r' || c == '\n')
		{
			c = '\0';
		}

		stdin_buffer[i] = c;

		// �f�o�b�O�p��echo
		if(is_echo)
		{
			if(c != '\0')
			{
				usart_putc(c);
			}else{
				usart_puts("\r\n");
			}
		}

		// null�����Ȃ當���񒊏o����
		if(c == '\0'){
			break;
		}
	}
	
	strncpy(buf, stdin_buffer, i);
	
	return buf;
}

#endif // _USART_USE_CHARACTER_FUNCTION