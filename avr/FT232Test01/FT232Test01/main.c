/**
 * @file   main.c
 * @brief  
 * @date   2017/03/25 21:28:53
 * @author Jackhammer
 */ 

//==============================================================================
// 
//  
//    �e�X�g�� (ATtine2313)
// 
// http://tsukulog555.blog14.fc2.com/blog-entry-73.html
// http://www.natural-science.or.jp/article/20101215012553.php
// http://d.hatena.ne.jp/hijouguchi/?of=59
// http://www.letstryit.net/2011/06/avr-rotaryencoder.html
// http://www.geocities.jp/kuman2600/k5rot-enc.html 2��3�����ւ���
// http://elm-chan.org/docs/tec/te01.html �`���^�����O�΍�
// http://startelc.com/H8/H8_51Encder1.html ��]���ʎ�
// http://www.chibiegg.net/elec/avr-elec/generate_pwm.htm sin�e�[�u��
// http://avrwiki.osdn.jp/cgi-bin/wiki.cgi?page=Timer0#p14
// http://usicolog.nomaki.jp/engineering/avr/avrInterrupt.html#howToUse ������blog
// ��˂��炨 AVR�V���A���ʐM�̃����O�o�b�t�@
// http://d.hatena.ne.jp/yaneurao/20080710
// http://d.hatena.ne.jp/yaneurao/20080713
//==============================================================================

//======================================
// system configuration.
//======================================
#include "config.h"

#include <stdint.h>				// types
#include <stdlib.h>
#include <string.h>

//#include <avr/iotn2313.h>
#include <avr/io.h>				// �s������
#include <avr/sfr_defs.h>		// _BV��
#include <avr/interrupt.h>		// ����������
#include <avr/wdt.h>			// WDT

#include <util/delay.h>			// _delay_ms��
#include <util/setbaud.h>		// F_CPU�ABAUD�錾�̃`�F�b�N

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

#define MIN(a,b)				(((a)<(b))?(a):(b))		//!< �ŏ��l��Ԃ�
#define MAX(a,b)				(((a)>(b))?(a):(b))		//!< �ő�l��Ԃ�
#define offsetof(s,m)			(size_t)&(((s *)0)->m)

//! �萔�֌W
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


//! �V���A�����痬��Ă��郁�b�Z�[�W�Q
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
#define MSG_OP_PIN				"pin"					//!< ping
#define APP_IDENTITY			"fkad"

#define PWM0A_ON(x)				{TCCR0A |= _BV(COM0A1);OCR0A = (x);}
#define PWM0A_OFF()				{TCCR0A ^= _BV(COM0A1);OCR0A = 0;}
#define GET_MPW()				(~((PIN_MPW>>PORT_MPW_BIT)))&1


//! message�̖���ID�ꗗ
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
		MSG_OP_ID_PKM,					//!< peak meter
		MSG_OP_ID_PIN,					//!< ping
		NUM_OF_MSG_OP_ID
} message_op;

const char *OPERATIONS[] = {
	MSG_OP_NOP,
	MSG_OP_OK,
	MSG_OP_ERR,
	MSG_OP_WAY,
	MSG_OP_IAM,
	MSG_OP_VER,
	MSG_OP_MIC,
	MSG_OP_MPW,
	MSG_OP_VOL,
	MSG_OP_PKM,
	MSG_OP_PIN
};


//! ���b�Z�[�W�̋��ʃt�H�[�}�b�g��`
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

//! �����̏�Ԃ��i�[����\����
typedef struct
{
	uint16_t	vol;					//!< ���ʃ{�����[��(10bit)
	uint16_t	mic;					//!< �}�C�N�{�����[��(10bit)
	uint8_t		mpw;					//!< �}�C�N�X�C�b�`���
	uint16_t	pkm;					//!< �s�[�N���[�^�[

} status;



volatile int timer				= COUNTER_LED;
typedef void (*PFUNC_usart_transmit)(const void*, unsigned char);



//=============================================================================
/**
 * @brief ���/����0��ꊄ�荞��
 */
//ISR(TIMER0_OVF_vect)
//{
//	PORT_LED ^=  _BV(PORT_LED_BIT);
//}

//=============================================================================
/**
 * @brief ���/����0��rB���荞��
 */
ISR(TIMER0_COMPB_vect)
{
	if(--timer > 0) return;
	timer = COUNTER_LED;

	OCR0B = (OCR0B-(256-COUNTER_LED_OP)+256)&0xFF;	// ��r�l�����Z���Ă������ƂŁACOUNTER_LED_OP�̃J�E���^�̑�p�ɂ���

	PORT_LED ^= _BV(PORT_LED_BIT);
}


//=============================================================================
/**
 * ������
 * @brief	������
 * @return	none
 */
static inline void initialize()
{
	/*
		NOTAMP�Ŏg�p����PORT����

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
		RTS			(IN)	PA1
		CTS			(OUT)	PA0
		
		(���g�p)
			PD4,PD6
			PB0
	*/

	//======================================
	// port configuration.
	//======================================
	DDR_LED	 |= _BV(PORT_LED_BIT);				// LED����|�[�g�̊Y��BIT���o�͐ݒ�ɁB
	PORT_LED &= ~_BV(PORT_LED_BIT);				// PORT_LED_BIT��LO�ɐݒ�

	DDR_PWM	 |= _BV(PORT_PWM_BIT);				// PWM�o�̓|�[�g�̊Y��BIT���o�͐ݒ�ɁB
	PORT_PWM &= ~_BV(PORT_PWM_BIT);				// PORT_PWM_BIT��LO�ɐݒ�

	DDR_MPW	 &= ~_BV(PORT_MPW_BIT);				// Mic Power�|�[�g����͂ɐݒ�
	PORT_MPW |= _BV(PORT_MPW_BIT);				// PORT_PWM_BIT���v���A�b�v

#if defined USE_RTSCTS
	DDR_CTSRTS	|= _BV(CTS_OUT);				// CTS���o�͂ɐݒ�
	DDR_CTSRTS	&= ~_BV(RTS_IN);				// RTS����͂ɐݒ�
	PORT_CTSRTS |= _BV(RTS_IN);					// RTS_IN���v���A�b�v
#endif

	DDR_MPW	 &= ~_BV(PORT_MPW_BIT);				// mic_power����͂ɐݒ�
	PORT_MPW |= _BV(PORT_MPW_BIT);				// mic_power���v���A�b�v


	//======================================
	// timer configuration.
	//======================================
	// TCCR0x���W�X�^�Őݒ肷�鍀��
	// TCCR0A COM0A1 COM0A0 COM0B1 COM0B0   -     -  WGM01 WGM00
	// TCCR0B FOC0A  FOC0B    -      -    WGM02 CS02 CS01  CS00
	
	// WGM(Wave Generation Mode)
	// WGM01=0,WGM00=0 => �W������		 : 
	// WGM01=1,WGM00=0 => CTC����		 : ��r��v���N�����ہATCNT��0���Z�b�g�����
	// WGM01=0,WGM00=1 => �ʑ��PWM����: TCNT0��0�`255�ŐU������.
	// WGM01=1,WGM00=1 => ����PWM����	 : TCNT0��0��255�ŉ�]����

	// COmpare Match 
	// COM0x1=0,COM0x0=0 => PWM�o�͖���
	// COM0x1=0,COM0x0=1 => ��r��v�Ńg�O��
	// COM0x1=1,COM0x0=0 => ��r��v��LO�ABOTTM��HI�@�@����ʓI
	// COM0x1=1,COM0x0=1 => ��L���]

	TCNT0	= 0;						// �^�C�}0�J�E���^�̏�����
	TCCR0A	= 0b00000011;				// ����PWM(PWM�o�͂�OC0A(PB2))
	TCCR0B	= PRESCALE_SEL;				// �N���b�N��1024����
	
	OCR0A	= 0;						// Timer0��A��rڼ޽�(PWM��duty��)
	OCR0B	= COUNTER_LED_OP;			// Timer0��B��rڼ޽�

	TIMSK	= _BV(OCIE0B);				// [TCNT0��B��r]��L����
	//TIMSK	= _BV(OCIE0B)|_BV(TOIE0);	// [TCNT0��B��r][TCNT0��OVF]��L����
	//======================================
	// usart configuration.
	//======================================
	usart_init();

	UCSRA	= 0;						// (USART Control and Status Register A):
	UCSRB	= _BV(RXEN)|_BV(TXEN);		// (USART Control and Status Register B):��M����,���M����(=>0b00011000)
	UCSRC	= 0b00000110;				// (USART Control and Status Register C):�񓯊��A8bit���f�[�^�A�p���e�B����
	
	sei();
}

//=============================================================================
/**
 * message��op��]����opid��ԋp
 * @brief	message��op��]����opid��ԋp
 * @param	(pmsg) �]���Ώۂ�message
 * @return	operation id
 */
 static inline char msg_get_opid(const message *pmsg){
	char			opid = MSG_OP_ID_NOP;
	const char			*pop;
	unsigned char	i;

	if(pmsg == NULL)
	{
		return opid;
	}

	pop = pmsg->op;

	for(i=NUM_OF_MSG_OP_ID; i-->0;)
	{
		if( memcmp(pop, OPERATIONS[i], sizeof(pmsg->op)) )
		{
			continue;
		}
			
		opid = i;
		break;
	}

	return opid;
}


//=============================================================================
/**
 * message��op���A�w�肵��opid���瓾����op�ŏ㏑��
 * @brief	message��op��opid���瓾����op�ŏ㏑��
 * @param	(pmsg) �]���Ώۂ�message
 * @param	(opid) operation id
 * @return	none
 */
static inline void msg_set_op(message *pmsg, char opid)
{
	memcpy(pmsg->op, OPERATIONS[(unsigned char)opid], sizeof(pmsg->op));
}

//=============================================================================
/**
 * �������񂩂�message���擾���A�ԋp����
 * @brief	����������message�擾
 * @param	(msg) �ԋp����message
 * @param	(pstr) ��ƂȂ镶����
 * @return	operation id
 */
char msg_get(message *pmsg, const char *pdata)
{
	if(pmsg == NULL)
	{
		return MSG_OP_ID_NOP;
	}

	// msg��������
	memcpy(pmsg, pdata, sizeof(*pmsg));
	
	// op������op-index�𓾂�
	return msg_get_opid(pmsg);
}


//=============================================================================
/**
 * DEBUG:message���V���A���ɏo�͂���
 * @brief	message���V���A���ɏo��
 * @param	(msg) �o�͑Ώۂ�message
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

//=============================================================================
/**
 * DEBUG:message���V���A���ɏo�͂���
 * @brief	message���V���A���ɏo��
 * @param	(msg) �o�͑Ώۂ�message
 * @return	none
 */
static inline void status_update(status* pstats){
	pstats->mic = 1;//(pstats->mic+1) & MIC_MASK;
	pstats->vol = 2;//(pstats->vol+2) & VOL_MASK;
	pstats->mpw = GET_MPW();
	//pstats->pkm = 0;
}


static inline char msg_make_response(message* pmsg, char opid, status* pstats)
{
	switch(opid)
	{
		case MSG_OP_ID_WAY:
			opid = MSG_OP_ID_IAM;
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
			
			opid = MSG_OP_ID_OK;
			memcpy(pmsg->val_c, APP_IDENTITY, sizeof(pmsg->val_c));
			break;
		default:
			opid = MSG_OP_ID_NOP;
	}
	
	msg_set_op(pmsg, opid);


	return opid;
}


void msg_make_and_send_response(message *pmsg, char opid, status *pstats){
	opid = msg_make_response(pmsg, opid, pstats);				//!< ��M�f�[�^�����ɕԐM�f�[�^���쐬

	//! �ԐM���K�v�ł���΁A�ԐM����
	if(opid != MSG_OP_ID_NOP)
	{
		usart_transmit_bytes(pmsg, (unsigned char)sizeof(*pmsg));	//!< �ԐM�f�[�^�𑗐M
	}
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
	status  stats_plv= {-1,-1,-1,-1};
	
	initialize();									//!< ������

	while(1)
	{
		usart_recieve_bytes(buf, sizeof(buf));		//!< ��M
		opid = msg_get(&msg, buf);					//!< message���
		
		//! msg�������ȏꍇ�͂�蒼��
		if(opid == MSG_OP_ID_NOP)
		{
			continue;
		}

		status_update(&stats);										//!< �X�e�[�^�X�X�V
		
		msg_make_and_send_response(&msg, opid, &stats);
		
		if(stats.mic != stats_plv.mic){
			msg_make_and_send_response(&msg, MSG_OP_ID_MIC, &stats);
		}
		if(stats.vol != stats_plv.vol){
			msg_make_and_send_response(&msg, MSG_OP_ID_VOL, &stats);
		}
		if(stats.mpw != stats_plv.mpw){
			msg_make_and_send_response(&msg, MSG_OP_ID_MPW, &stats);
		}

		memcpy(&stats_plv, &stats, sizeof(stats));

		//msg_put_debug(&msg);
	}

	return 0;
}
