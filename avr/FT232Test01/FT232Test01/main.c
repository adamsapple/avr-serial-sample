/**
 * @file   main.c
 * @brief  
 * @date   2017/03/25 21:28:53
 * @author Jackhammer
 */ 

//==============================================================================
// 
//  
//    テスト版 (ATtine2313)
// 
// http://tsukulog555.blog14.fc2.com/blog-entry-73.html
// http://www.natural-science.or.jp/article/20101215012553.php
// http://d.hatena.ne.jp/hijouguchi/?of=59
// http://www.letstryit.net/2011/06/avr-rotaryencoder.html
// http://www.geocities.jp/kuman2600/k5rot-enc.html 2と3を入れ替える
// http://elm-chan.org/docs/tec/te01.html チャタリング対策
// http://startelc.com/H8/H8_51Encder1.html 回転判別式
// http://www.chibiegg.net/elec/avr-elec/generate_pwm.htm sinテーブル
// http://avrwiki.osdn.jp/cgi-bin/wiki.cgi?page=Timer0#p14
// http://usicolog.nomaki.jp/engineering/avr/avrInterrupt.html#howToUse うしこblog
//==============================================================================

//======================================
// system configuration.
//======================================
#include "config.h"

#include <stdint.h>				// types
#include <stdlib.h>
#include <string.h>

//#include <avr/iotn2313.h>
#include <avr/io.h>				// ピン名等
#include <avr/sfr_defs.h>		// _BV等
#include <avr/interrupt.h>		// 割込処理等
#include <avr/wdt.h>			// WDT

#include <util/delay.h>			// _delay_ms等
#include <util/setbaud.h>		// F_CPU、BAUD宣言のチェック

#include "usart.h"

#define	PRESCALE_1				0b001;
#define	PRESCALE_8				0b010;
#define	PRESCALE_64				0b011;
#define	PRESCALE_256			0b100;
#define	PRESCALE_1024			0b101;

#define PRESCALE_SEL			PRESCALE_256
#define COUNTER_1S				(31<<2)
#define COUNTER_500MS			(COUNTER_1S >>1)
#define COUNTER_LED				COUNTER_500MS
#define COUNTER_LED_OP			(249)

#define DDR_LED					DDRB
#define PORT_LED				PORTB
#define PORT_LED_BIT			PB0

#define DDR_PWM					DDRB
#define PORT_PWM				PORTB
#define PORT_PWM_BIT			PB2

#define	DDR_MPW					DDRB
#define	PORT_MPW				PORTB
#define	PORT_MPW_BIT			PB3
#define	PIN_MPW					PINB




#ifdef DEBUG
#	define _delay_ms(x)			asm("nop")
#	define _delay_us(x)			asm("nop")
#	undef  COUNTER_LED
#	define COUNTER_LED			1
#endif

#define MIN(a,b)				(((a)<(b))?(a):(b))		//!< 最小値を返す
#define MAX(a,b)				(((a)>(b))?(a):(b))		//!< 最大値を返す
#define offsetof(s,m)			(size_t)&(((s *)0)->m)

//! 定数関係
#define MIC_BIT_WIDTH			10
#define MIC_MAX					((1<<MIC_BIT_WIDTH)-1)
#define MIC_MASK				MIC_MAX

#define VOL_BIT_WIDTH			10
#define VOL_MAX					((1<<VOL_BIT_WIDTH)-1)
#define VOL_MASK				VOL_MAX

#define PKM_BIT_WIDTH			10
#define PKM_MAX					((1<<PKM_BIT_WIDTH)-1)
#define PKM_MASK				PKM_MAX

#define PWM_DUTY_BIT_WIDTH		8
#define PWM_DUTY_MAX			((1<<PWM_DUTY_BIT_WIDTH)-1)
#define PWM_DUTY_MASK			PWM_DUTY_MAX

//! シリアルから流れてくるメッセージ群
#define MSG_OP_NOP				"nop"					//!< no operation
#define MSG_OP_OK				"ok_"					//!< ok
#define MSG_OP_ERR				"err"					//!< err
#define MSG_OP_WAY				"way"					//!< who are you
#define MSG_OP_IAM				"iam"					//!< i am
#define MSG_OP_VER				"ver"					//!< version
#define MSG_OP_MIC				"mic"					//!< mic
#define MSG_OP_MPW				"mpw"					//!< mic power
#define MSG_OP_VOL				"vol"					//!< volume
#define MSG_OP_PKM				"pkm"					//!< peak meter
#define APP_IDENTITY			"fkad"

#define PWM0A_ON(x)				{TCCR0A |= _BV(COM0A1);OCR0A = (x);}
#define PWM0A_OFF()				{TCCR0A ^= _BV(COM0A1);OCR0A = 0;}
#define GET_MPW()				((PIN_MPW>>PORT_MPW_BIT)&1)


//! messageの命令ID一覧
enum {
		MSG_OP_ID_NOP,					//!< no operation
		MSG_OP_ID_OK,					//!< ok
		MSG_OP_ID_ERR,					//!< error
		MSG_OP_ID_WAY,					//!< who are you
		MSG_OP_ID_IAM,					//!< i am
		MSG_OP_ID_VER,					//!< version
		MSG_OP_ID_MIC,					//!< mic
		MSG_OP_ID_MPW,					//!< mic power
		MSG_OP_ID_VOL,					//!< volume
		MSG_OP_ID_PKM 					//!< peak meter
} message_op;

//! メッセージの共通フォーマット定義
typedef struct
{
	char		op[3];					//!< 0-2:operator
	union
	{
		char val_c[4];					//!< 3-6:chars value
		struct
		{
			int16_t val_i_a;			//!< 3-4:int16 value
			int8_t	val_i_b;			//!< 5  :int8  value
			int8_t	val_i_c;			//!< 6  :int8  value
		};
	};
	char		reserve[1];				//!< 7  :reserve
} message;

//! 内部の状態を格納する構造体
typedef struct
{
	uint16_t	vol;					//!< 音量ボリューム(10bit)
	uint16_t	mic;					//!< マイクボリューム(10bit)
	uint8_t		mpw;					//!< マイクスイッチ状態
	uint16_t	pkm;					//!< ピークメーター

} status;



volatile int timer				= COUNTER_LED;
typedef void (*PFUNC_usart_transmit)(const void*, unsigned char);



//=============================================================================
/**
 * @brief ﾀｲﾏ/ｶｳﾝﾀ0溢れ割り込み
 */
//ISR(TIMER0_OVF_vect)
//{
//	PORT_LED ^=  _BV(PORT_LED_BIT);
//}

//=============================================================================
/**
 * @brief ﾀｲﾏ/ｶｳﾝﾀ0比較B割り込み
 */
ISR(TIMER0_COMPB_vect)
{
	if(--timer > 0) return;
	timer = COUNTER_LED;

	OCR0B = (OCR0B-(256-COUNTER_LED_OP)+256)&0xFF;	// 比較値を減算していくことで、COUNTER_LED_OPのカウンタの代用にする

	PORT_LED ^= _BV(PORT_LED_BIT);
}


//=============================================================================
/**
 * 初期化
 * @brief	初期化
 * @return	none
 */
static inline void initialize()
{
	/*
		NOTAMPで使用するPORT検討

		SPI_USCK	(OUT)	PB7
		SPI_MISO	(IN)	PB6
		SPI_MOSI	(OUT)	PB5
		SPI_SS0		(OUT)	PB4
		MIC_PWR_IN	(IN)	PB3(PCINT3)
		PMT_PWM		(--)	PB2(OC0A)
		RELAY_SWT	(OUT)	PB1
		RESET		(--)	PA2
		USART_RX	(--)	PD0
		USART_TX	(--)	PD1
		LED_LISTEN	(OUT)	PD2
		LED_CONNECT	(OUT)	PD3
		LED_PING	(OUT)	PD5
		
		(未使用)
			PA0,PA1
			PD4,PD6
			PB0
	*/

	//======================================
	// port configuration.
	//======================================
	DDR_LED	 |= _BV(PORT_LED_BIT);				// LED制御ポートの該当BITを出力設定に。
	PORT_LED &= ~_BV(PORT_LED_BIT);				// PORT_LED_BITをLOに設定

	DDR_PWM	 |= _BV(PORT_PWM_BIT);				// PWM出力ポートの該当BITを出力設定に。
	PORT_PWM &= ~_BV(PORT_PWM_BIT);				// PORT_PWM_BITをLOに設定

	DDR_MPW	 &= ~_BV(PORT_MPW_BIT);				// Mic Powerポートを入力に設定
	PORT_MPW |= _BV(PORT_MPW_BIT);				// PORT_PWM_BITをプルアップ

	//======================================
	// timer configuration.
	//======================================
	// TCCR0xレジスタで設定する項目
	// TCCR0A COM0A1 COM0A0 COM0B1 COM0B0   -     -  WGM01 WGM00
	// TCCR0B FOC0A  FOC0B    -      -    WGM02 CS02 CS01  CS00
	
	// WGM(Wave Generation Mode)
	// WGM01=0,WGM00=0 => 標準動作		 : 
	// WGM01=1,WGM00=0 => CTC動作		 : 比較一致が起きた際、TCNTが0リセットされる
	// WGM01=0,WGM00=1 => 位相基準PWM動作: TCNT0が0～255で振幅する.
	// WGM01=1,WGM00=1 => 高速PWM動作	 : TCNT0が0⇒255で回転する

	// COmpare Match 
	// COM0x1=0,COM0x0=0 => PWM出力無し
	// COM0x1=0,COM0x0=1 => 比較一致でトグル
	// COM0x1=1,COM0x0=0 => 比較一致でLO、BOTTMでHI　　※一般的
	// COM0x1=1,COM0x0=1 => 上記反転

	TCNT0	= 0;						// タイマ0カウンタの初期化
	TCCR0A	= 0b00000011;				// 高速PWM(PWM出力はOC0A(PB2))
	TCCR0B	= PRESCALE_SEL;				// クロックを1024分周
	
	OCR0A	= 0;						// Timer0のA比較ﾚｼﾞｽﾀ(PWMのduty比=50%)
	OCR0B	= COUNTER_LED_OP;			// Timer0のB比較ﾚｼﾞｽﾀ

	TIMSK	= _BV(OCIE0B);				// [TCNT0のB比較]を有効化
	//TIMSK	= _BV(OCIE0B)|_BV(TOIE0);	// [TCNT0のB比較][TCNT0のOVF]を有効化
	//======================================
	// usart configuration.
	//======================================
	usart_init();

	UCSRA	= 0;						// (USART Control and Status Register A):
	UCSRB	= _BV(RXEN)|_BV(TXEN);		// (USART Control and Status Register B):受信許可,送信許可(=>0b00011000)
	UCSRC	= 0b00000110;				// (USART Control and Status Register C):非同期、8bit長データ、パリティ無し
	
	sei();
}


//=============================================================================
/**
 * 文字列情報からmessageを取得し、返却する
 * @brief	文字列を基にmessage取得
 * @param	(msg) 返却するmessage
 * @param	(pstr) 基となる文字列
 * @return	operation id
 */
char msg_get(message *pmsg, const char *pdata)
{
	char	opid = MSG_OP_ID_NOP;
	char	*pop;

	if(pmsg == NULL)
	{
		return opid;
	}

	// msgを初期化
	memcpy(pmsg, pdata, sizeof(*pmsg));
	pop = pmsg->op;
	
	// op部からop-indexを得る
	if(memcmp(pop, MSG_OP_WAY, sizeof(pmsg->op)) == 0){
		opid = MSG_OP_ID_WAY;
	}else if(memcmp(pop, MSG_OP_VER, sizeof(pmsg->op)) == 0){
		opid = MSG_OP_ID_VER;
	}else if(memcmp(pop, MSG_OP_MIC, sizeof(pmsg->op)) == 0){
	opid = MSG_OP_ID_MIC;
	}else if(memcmp(pop, MSG_OP_MPW, sizeof(pmsg->op)) == 0){
	opid = MSG_OP_ID_MPW;
	}else if(memcmp(pop, MSG_OP_VOL, sizeof(pmsg->op)) == 0){
		opid = MSG_OP_ID_VOL;
	}else if(memcmp(pop, MSG_OP_PKM, sizeof(pmsg->op)) == 0){
		opid = MSG_OP_ID_PKM;
	}

	return opid;
}


//=============================================================================
/**
 * DEBUG:messageをシリアルに出力する
 * @brief	messageをシリアルに出力
 * @param	(msg) 出力対象のmessage
 * @return	none
 */
void msg_put_debug(const message* pmsg){
	usart_puts("op :");
	usart_transmit_bytes(pmsg->op, sizeof(pmsg->op));
	usart_puts("\r\n");
	usart_puts("val:");
	//usart_puts(itoa(msg->, buf, 10));
	usart_puts("\r\n");
}

static inline void status_update(status* pstats){
	pstats->mic = (pstats->mic+1) & MIC_MASK;
	pstats->vol = (pstats->vol+2) & VOL_MASK;
	pstats->mpw = GET_MPW();
	//pstats->pkm = 0;
}

static inline char msg_make_response(message* pmsg, char opid, status* pstats)
{
	switch(opid)
	{
		case MSG_OP_ID_WAY:
			opid = MSG_OP_ID_IAM;
			memcpy(pmsg->op, MSG_OP_IAM, sizeof(pmsg->op));
			memcpy(pmsg->val_c, APP_IDENTITY, sizeof(pmsg->val_c));
			break;
		case MSG_OP_ID_VER:
			pmsg->val_i_a = 1;
			//pmsg->val_i_b = 0;
			//pmsg->val_i_c = 0;
			break;
		case MSG_OP_ID_MIC:
			pmsg->val_i_a = MIN(MAX(0, pstats->mic), MIC_MAX);
			//pmsg->val_i_b = 0;
			//pmsg->val_i_c = 0;
			break;
		case MSG_OP_ID_MPW:
			pmsg->val_i_a = MIN(MAX(0, pstats->mpw), 1);
			//pmsg->val_i_b = 0;
			//pmsg->val_i_c = 0;
			break;
		case MSG_OP_ID_VOL:
			pmsg->val_i_a = MIN(MAX(0, pstats->vol), VOL_MAX);
			//pmsg->val_i_b = 0;
			//pmsg->val_i_c = 0;
			break;
		case MSG_OP_ID_PKM:
			pstats->pkm = MIN(MAX(0, pmsg->val_i_a), PKM_MAX);
			
			if(pstats->pkm > 0)
			{
				PWM0A_ON(pstats->pkm>>(PKM_BIT_WIDTH-PWM_DUTY_BIT_WIDTH));
			}else{
				PWM0A_OFF();
			}
			
			opid = MSG_OP_ID_NOP;
			
			//opid = MSG_OP_ID_OK;
			//memcpy(pmsg->op, MSG_OP_OK, sizeof(pmsg->op));
			//memcpy(pmsg->val_c, APP_IDENTITY, sizeof(pmsg->val_c));
			break;
		default:
			opid = MSG_OP_ID_NOP;
	}
	
	return opid;
}

//=============================================================================
//
//! main.
//
//=============================================================================
int main()
{
	char	buf[sizeof(message)];
	char    opid;
	message	msg;
	status  stats	 = {0};
	
	initialize();									//!< 初期化

	while(1)
	{
		usart_recieve_bytes(buf, sizeof(buf));		//!< 受信
		opid = msg_get(&msg, buf);					//!< message解析
		
		//! msgが無効な場合はやり直し
		if(opid == MSG_OP_ID_NOP)
		{
			continue;
		}

		status_update(&stats);										//!< ステータス更新
		opid = msg_make_response(&msg, opid, &stats);				//!< 受信データを元に返信データを作成
		
		//! 返信が必要であれば、返信する
		if(opid != MSG_OP_ID_NOP)
		{
			usart_transmit_bytes(&msg, (unsigned char)sizeof(msg));	//!< 返信データを送信 
		}

		//msg_put_debug(&msg);
	}

	return 0;
}
