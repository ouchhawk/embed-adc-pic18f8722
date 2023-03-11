#include <p18cxxx.h>
#include <p18f8722.h>
#include <xc.h>
#include "Includes.h"
#include "LCD.h"
#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF
#define _XTAL_FREQ   40000000

unsigned char c1, a = '#' , b = '#', c = '#',d='#';
unsigned int POT=10, d_voltage=1060; // Converted Potentiometer Value [0-9]
char PIN[4]="####";

int address_counter = 0x8B, test_counter=0; // "Selected Digit" Address for "Set Pin" Task
int digit_no=0; // PIN's Digit Position
int not_set=1; // PIN assignment
int blink_chr=0, blink_pin=0, show_chr=0, show_pin=0; // Flags for blink tasks
int blink_counter=0;
int attempts =2;
int t0_counter0=0;
int t0_counter1=0;
int t0_counter2=0;
int t1_counter=0;
int temp_voltage=0;
    
void interrupt ISR()
{
    if(INTCONbits.TMR0IE && INTCONbits.TMR0IF) {
        t0_counter1++;
        t0_counter2++;
        blink_counter++;
        TMR0=0; 
		
        if(t0_counter0>1){ 
            t0_counter0=0;
            ADCON0bits.GO=1;
        }
        if(t0_counter1>0x32 && blink_chr==1) // 250ms, BLINK DIGIT
        {   
            t0_counter1=0;
            
            if(show_chr==1)
            {
                updateLCD(PIN[digit_no],address_counter);
                show_chr=0;
            }
            else if (show_chr==0)
            {
                updateLCD(' ',address_counter);
                show_chr=1;
            }
        }
            
        if(blink_counter>62 && blink_pin==1)
        { 
            blink_counter=0;
            
            if(show_pin==1)
            {
            WriteCommandToLCD(0x80); 
            WriteStringToLCD(" The new pin is ");
            WriteCommandToLCD(0xC0);
            WriteStringToLCD("   ---    ---   ");

            WriteCommandToLCD(0xC6); 
            WriteDataToLCD(PIN[0]);
            WriteCommandToLCD(0xC7); 
            WriteDataToLCD(PIN[1]);
            WriteCommandToLCD(0xC8); 
            WriteDataToLCD(PIN[2]);
            WriteCommandToLCD(0xC9); 
            WriteDataToLCD(PIN[3]);
            show_pin=0;
            }
            else if (show_pin==0)
            {
            ClearLCDScreen();  
            show_pin=1;
            }
        }     
    }
 
    //-------------RB INTERRUPT-------------      
    if (INTCONbits.RBIF && INTCONbits.RBIE) 
    {
        INTCONbits.RBIF=0; 

        if (PORTBbits.RB6==0 && digit_no<3) // GO NEXT PIN DIGIT
        {
            test_counter++;
            if (test_counter%2)
            {
            address_counter++;
            digit_no++;
            blink_chr=1;
            }
        }

        else if (PORTBbits.RB7==0 && digit_no>=3 ) // COMPLETE SETTINGS
        {
            blink_pin=1;
            show_pin=1;
            PIE1bits.ADIE=0;
        }
    }   
    
    //-------------ADC HANDLE-------------    
    
    if(PIE1bits.ADIE && PIR1bits.ADIF && digit_no<4) // COLLECT ADC VALUE
    {
        test_counter++;
        if (test_counter%2)
        {

            PIR1bits.ADIF=0;
            d_voltage=ADRESL|(((unsigned int)(ADRESH))<<8);
            //d_voltage=(ADRESH*(2^8))+ADRESL;
            
            if (d_voltage!=temp_voltage)
            {
                temp_voltage=d_voltage;
                blink_chr=0;
                
                if(d_voltage>=0 && d_voltage<=99) POT=0;
                else if(d_voltage>=100 && d_voltage<=199) POT=1;
                else if(d_voltage>=200 && d_voltage<=299) POT=2;
                else if(d_voltage>=300 && d_voltage<=399) POT=3;
                else if(d_voltage>=400 && d_voltage<=499) POT=4;
                else if(d_voltage>=500 && d_voltage<=599) POT=5;
                else if(d_voltage>=600 && d_voltage<=699) POT=6;
                else if(d_voltage>=700 && d_voltage<=799) POT=7;
                else if(d_voltage>=800 && d_voltage<=899) POT=8;
                else if(d_voltage>=900 && d_voltage<=1024) POT=9; 
                updateDigit();
            }
        }  
    }
    INTCONbits.TMR0IF=0;
}



void STATE1()
{    
    ClearLCDScreen();          
    WriteCommandToLCD(0x80); 
    WriteStringToLCD(" $>Very  Safe<$ ");
    WriteCommandToLCD(0xC0);
    WriteStringToLCD(" $$$$$$$$$$$$$$ ");
}

void STATE2()
{
    WriteCommandToLCD(0x80);
    WriteStringToLCD(" Set a pin:#### ");   
    WriteCommandToLCD(0xC0);
    WriteStringToLCD("                "); 
    //WriteCommandToLCD(address_counter); // Select DDRAM Address to blink
    //WriteCommandToLCD(0x0D); // DISPLAY=ON, CURSOR=OFF, BLINK=ON
    
    //---------------PIN SETTING PERIOD------
    
    ADCON0=0b00110000; // CHS=12
    ADCON1=0b00000000; // Analog Input
    ADCON2=0b10001010; // Right Justified
    INTCON=0b00000000; // RBIF INT0IE TMR0IE

    T0CON=0b01000111; // Prescaler=1:256, T0CS
    T1CON=0b11110100;
            
    TMR0=0;
    TMR1=0;
    
    // START
    INTCONbits.PEIE=1; // Enable Peripheral Interrupts
    INTCONbits.GIE=1; // Enable Global Interrupts
    INTCON2bits.RBPU=0;
    RCON=0;
    PIR1=0;
    PIE1=0;
    IPR1=0;
    IPR1bits.TMR1IP=1;
        
    PIE1bits.ADIE=1; //  Enable AD Interrupts
    PIR1bits.ADIF=0; // Reset AD Interrupt Flag
    
    ADCON0bits.ADON=1; // Enable AD Converter
    ADCON0bits.GO=1; // Start AD Converter
    
    PIR1bits.TMR1IF=0; // Reset T1 Interrupt Flag
    PIE1bits.TMR1IE=1; // Enable T1 Interrupt
    T1CONbits.TMR1ON=1;
    
    INTCONbits.T0IE=1; // Enable T0 Interrupt 
    INTCONbits.T0IF=0; // Reset T0 Interrupt Flag
    
    T0CONbits.TMR0ON=1; 
	
    //RCONbits.IPEN=1; // Enable Interrupt Priority
    //IPR1bits.TMR1IP=1; // TMR1 Has High Priority
    
    INTCONbits.RBIF=0; // Reset RB Interrupt Flag 
    INTCONbits.RBIE=1; // Enable RB Interrupt
	
    //T1CON=0b11110101; // T1SYNC, 
    INTCONbits.RBIE=1; // Enable RB Interrupt
}


void STATE3()
{
    int k=0;
    while(1)
    {
        k++;
    }
    //---------------TEST PERIOD-------- 
}


void updateDigit()
{
    switch(POT) // Converted Potentiometer Value [0-9]
    {
        case 0:
            PIN[digit_no]='0';
            updateLCD('0',address_counter);
            blink_chr=0;
            break;

        case 1:
            PIN[digit_no]='1';
            updateLCD('1',address_counter);
            blink_chr=0;
            break;

        case 2:
            PIN[digit_no]='2';
            updateLCD('2',address_counter);
            blink_chr=0;
            break;

        case 3:
            PIN[digit_no]='3';
            updateLCD('3',address_counter);
            blink_chr=0;
            break;

        case 4:
            PIN[digit_no]='4';
            updateLCD('4',address_counter);
            blink_chr=0;
            break;

        case 5:
            PIN[digit_no]='5';
            updateLCD('5',address_counter);
            blink_chr=0;
            break;

        case 6:
            PIN[digit_no]='6';
            updateLCD('6',address_counter);
            blink_chr=0;
            break;

        case 7:
            PIN[digit_no]='7';
            updateLCD('7',address_counter);
            blink_chr=0;
            break;

        case 8:
            PIN[digit_no]='8';
            updateLCD('8',address_counter);
            blink_chr=0;
            break;

        case 9:
            PIN[digit_no]='9';
            updateLCD('9',address_counter);
            blink_chr=0;
            break;

        default:
            break;
    }
}

void main(void)
{
    InitPORTS();
    InitLCD();

    STATE1(); // INTRO
    STATE2(); // PIN SETTING PERIOD
    STATE3(); // TEST PERIOD
}
