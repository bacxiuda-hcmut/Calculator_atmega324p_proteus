#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>

//================ LCD =================
#define RS PD0
#define RW PD1
#define EN PD2

void LCD_Enable()
{
	PORTD |= (1<<EN);
	_delay_us(1);
	PORTD &= ~(1<<EN);
	_delay_ms(2);
}

void LCD_Send4Bit(uint8_t data)
{
	PORTD &= 0x0F;
	PORTD |= (data & 0xF0);
	LCD_Enable();
}

void LCD_Command(uint8_t cmd)
{
	PORTD &= ~(1<<RS);
	PORTD &= ~(1<<RW);
	LCD_Send4Bit(cmd);
	LCD_Send4Bit(cmd<<4);
}

void LCD_Char(char data)
{
	PORTD |= (1<<RS);
	PORTD &= ~(1<<RW);
	LCD_Send4Bit(data);
	LCD_Send4Bit(data<<4);
}

void LCD_String(char *str)
{
	while(*str) LCD_Char(*str++);
}

void LCD_Clear()
{
	LCD_Command(0x01);
	_delay_ms(2);
}

void LCD_Init()
{
	DDRD = 0xFF;
	_delay_ms(50);
	LCD_Command(0x02);
	LCD_Command(0x28);
	LCD_Command(0x0C);
	LCD_Command(0x06);
	LCD_Command(0x01);
}

//================ KEYPAD =================

char keypad[4][6] = {
	{'C','7','8','9','*','/'},
	{'S','4','5','6','-','M'},
	{'%','1','2','3','+','N'},
	{'R','0','.','=','+','P'}
};

void Keypad_Init()
{
	DDRA = 0x0F;
	PORTA = 0xFF;
	DDRB &= ~0x03;
	PORTB |= 0x03;
}

char GetKey()
{
	for(uint8_t r=0; r<4; r++)
	{
		PORTA |= 0x0F;
		PORTA &= ~(1<<r);
		_delay_us(5);

		for(uint8_t c=0; c<4; c++)
		{
			if(!(PINA & (1<<(c+4))))
			{
				_delay_ms(20);
				while(!(PINA & (1<<(c+4))));
				return keypad[r][c];
			}
		}

		for(uint8_t c=0; c<2; c++)
		{
			if(!(PINB & (1<<c)))
			{
				_delay_ms(20);
				while(!(PINB & (1<<c)));
				return keypad[r][c+4];
			}
		}
	}
	return 0;
}

//================ CALCULATOR =================

double num1=0, num2=0, memory=0;
char op=0;
uint8_t state=0;
uint8_t justCalculated=0;

void reset_calc()
{
	num1=0; num2=0; op=0;
	state=0;
	justCalculated=0;
	LCD_Clear();
}

double calculate()
{
	switch(op)
	{
		case '+': return num1 + num2;
		case '-': return num1 - num2;
		case '*': return num1 * num2;
		case '/': return (num2!=0)? num1/num2:0;
	}
	return num1;
}

//================ MAIN =================

int main(void)
{
	LCD_Init();
	Keypad_Init();

	char buf[16];
	LCD_String("READY");

	while(1)
	{
		char key = GetKey();

		if(key)
		{
			// RESET
			if(key=='C')
			{
				reset_calc();
			}

			// NUMBER
			else if(key>='0' && key<='9')
			{
				if(justCalculated)
				{
					reset_calc();
				}

				LCD_Char(key);

				if(state==0)
				num1 = num1*10 + (key-'0');
				else
				num2 = num2*10 + (key-'0');

				justCalculated = 0;
			}

			// OPERATOR
			else if(key=='+'||key=='-'||key=='*'||key=='/')
			{
				if(state==1) // chain calculation
				{
					num1 = calculate();
					num2 = 0;

					LCD_Clear();
					dtostrf(num1,8,2,buf);
					LCD_String(buf);
				}

				op = key;
				state = 1;
				justCalculated = 0;

				LCD_Char(key);
			}

			// EQUAL
			else if(key=='=')
			{
				double result = calculate();
				dtostrf(result,8,2,buf);

				LCD_Clear();
				LCD_String(buf);

				num1 = result;
				num2 = 0;
				state = 0;
				justCalculated = 1;
			}

			// SQRT
			else if(key=='R')
			{
				num1 = sqrt(num1);
				dtostrf(num1,8,2,buf);
				LCD_Clear();
				LCD_String(buf);

				justCalculated = 1;
			}

			// %
			else if(key=='%')
			{
				num1 = num1 / 100.0;
				dtostrf(num1,8,2,buf);
				LCD_Clear();
				LCD_String(buf);

				justCalculated = 1;
			}

			// +/- (??i d?u)
			else if(key=='S')
			{
				num1 = -num1;
				dtostrf(num1,8,2,buf);
				LCD_Clear();
				LCD_String(buf);
			}

			// MEMORY +
			else if(key=='P')
			{
				memory += num1;
			}

			// MEMORY -
			else if(key=='N')
			{
				memory -= num1;
			}

			// MEMORY RECALL
			else if(key=='M')
			{
				num1 = memory;
				dtostrf(num1,8,2,buf);
				LCD_Clear();
				LCD_String(buf);

				justCalculated = 1;
			}
		}
	}
}