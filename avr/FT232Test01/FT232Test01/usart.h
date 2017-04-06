 /**
 * @file   usart.h
 * @brief  
 * @date   2017/03/26 17:34:56
 * @author Jackhammer
 */ 
 
/*
 USARTについて:
  * UBRR0 = 動作周波数 / (ボーレート * 16) - 1
	ボーレート : 2400
	動作周波数 : 1M
	ならば
	UBRR0 = 25
  * UCSR0B : 受信・送信有効にする設定
	000  1    　　1       000
	受信有効　送信有効
  * UCSR0C : 非同期動作、データビット長を8bitとして設定
	00000110
  * UDR0   : 送受信するデータのレジスタ
  * UCSR0AのUDRE : 送信準備OKなら1になる
	準備OK -> 00100000
  * UCSR0AのRXC  : 受信準備OKなら1になる
	受信 -> 10000000
 see:
 https://plaza.rakuten.co.jp/ecircuit30/diary/200810180000/
 うしこblog USART http://usicolog.nomaki.jp/engineering.html
 始める電子回路(AVR) http://startelc.com/AVR/Avr_16trsUSB232c.html
 UARTとUSARTの違い http://wa3.i-3-i.info/diff193uart.html
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
#include <avr/sfr_defs.h>		// _BV等



//=============================================================================
/**
 * シリアル通信にて1byte受信する
 * @brief	シリアル通信にて1byte受信する
 * @return	1byte
 */
static inline uint8_t usart_recieve( void )
{
	loop_until_bit_is_set(UCSRA, RXCIE);		// 受信完了待ち:受信が完了するとUCSRAのRXCIE(7bit)が1になる
	
	return UDR;
}


//=============================================================================
/**
 * シリアル通信にて1byte送信する
 * @brief	シリアル通信にて1byte送信する
 * @param	送信データ
 * @return	none
 */
static inline void usart_transmit( uint8_t data )
{
	loop_until_bit_is_set(UCSRA, UDRIE);		// バッファ待機待ち:送信が完了しバッファが空になるとUCSRAのUDRIE(5bit)が1になる
	
	UDR = data;
}


//=============================================================================
/**
 * シリアル通信にて送信が完了するまで待機する。同期的な処理が必要な際利用する。
 * @brief	シリアル通信にて送信が完了するまで待機する
 * @return	none
 */
static inline void usart_flush( void )
{
	loop_until_bit_is_set(UCSRA, TXCIE);		// 送信完了待ち:送信が完了するとUCSRAのTXCIE(6bit)が1になる
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
