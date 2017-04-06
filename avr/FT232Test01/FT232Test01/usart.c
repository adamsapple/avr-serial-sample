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
#include <avr/sfr_defs.h>		// _BV等
#include <util/setbaud.h>		// F_CPU、BAUD宣言のチェック

//! シリアルからの入力バッファ
char *stdin_buffer;//[16];
//! エコー出力する場合は1
char is_echo = 1;


#if 0

// 割り込みによる送信
// 送信シフトレジスタが空になり、1フレームの送信が終わった時に発生
ISR(USART_TX_vect)
{
	usart_private_send_char();
}

// 割込みによる送信
ISR(USART_RX_vect)
{
	usart_recv.buf[usart_recv.seek_write] = UDR;    // 受信データを受信バッファに格納
	cli();
	INC_LOOP(usart_recv.seek_write, USART_BUFF_SIZE_MASK);
	sei();
}

// UDRが空いたときに発生
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
 * シリアルからバイナリデータ列を取得する
 * @brief	シリアルからバイナリデータを取得する
 * @param	(pdata) データ格納先
 * @param	(size) 出力サイズ(bytes)
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
 * シリアルへバイナリデータ列を出力する
 * @brief	バイナリデータを出力する
 * @param	(pdata) 出力するデータ
 * @param	(size) 出力サイズ(bytes)
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
 * FCPU,BAUDの設定値を基にusartのボーレート等の設定を行う。
 * @brief	usart初期化
 * @return	none
 */
void usart_init()
{
	//stdout = &usart_output;
	//stdin  = &usart_input;

	UBRRH	= UBRRH_VALUE;				// (USART Baud Rate Register Hi)
	UBRRL	= UBRRL_VALUE;				// (USART Baud Rate Register Lo) ※2313データシートのP88,89を参照
}

#if defined _USART_USE_CHARACTER_FUNCTION

//=============================================================================
/**
 * シリアルから1文字取得する
 * @brief	シリアル入力を1文字取得する
 * @return	1文字
 */
char usart_getc()
{
	return usart_recieve();
}


//=============================================================================
/**
 * シリアルへ1文字出力する
 * @brief	シリアルへ1文字出力する
 * @param	(c) 出力する文字
 * @return	none
 */
void usart_putc(char c)
{
	usart_transmit(c);
}


//=============================================================================
/**
 * シリアルへ文字列を出力する
 * @brief	シリアルへ文字列を出力する
 * @param	(pstr) 出力する文字列
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
 * シリアル入力を文字列として返す。
 * 改行(\r\n)は含まない、null終端付き。最大31+1文字。
 * @brief	シリアル入力を文字列として返す。
 * @param	(buf) 返却先文字列
 * @param	(n) bufのサイズ
 * @return	none
 */
char* usart_gets(char *buf, unsigned char n)
{
	unsigned char c, i = 0;

	memset(buf, 0, n);


	for(i=0; i<n; ++i)
	{
		// バッファ終端じゃなければ入力待ち
		// 終端なら自動でnull文字
		if(i < n-1)
		{
			c = usart_recieve();
		}else{
			c = '\0';
		}
		
		// 改行コードはnull文字扱い
		if(c == '\r' || c == '\n')
		{
			c = '\0';
		}

		stdin_buffer[i] = c;

		// デバッグ用のecho
		if(is_echo)
		{
			if(c != '\0')
			{
				usart_putc(c);
			}else{
				usart_puts("\r\n");
			}
		}

		// null文字なら文字列抽出完了
		if(c == '\0'){
			break;
		}
	}
	
	strncpy(buf, stdin_buffer, i);
	
	return buf;
}

#endif // _USART_USE_CHARACTER_FUNCTION