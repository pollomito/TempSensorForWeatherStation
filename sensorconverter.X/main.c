/*
 * File:   main.c
 * Author: Paolo
 *
 * Created on 20. aprile 2015, 22:04
 */

#define _XTAL_FREQ 16000000 // 500000


#include <stdio.h>
#include <stdlib.h>


#include "configurationbits.h"


#define PIN_CS      PORTAbits.RA4
#define PIN_DATA    PORTAbits.RA5
#define PIN_AM2302  PORTAbits.RA2
#define DIR_AM2302  TRISAbits.TRISA2  //1 in, 0 out

#define PORT_IN    1
#define PORT_OUT   0

#define delay_us(x)  __delay_us(x)
#define delay_ms(x)  __delay_ms(x)

short Time_out ;
int   Temp, RH;



unsigned char MakeCRC(unsigned char *BitString, unsigned char len)
{


  unsigned char crc = 0;
  int  i;
  char DoInvert;
  
  
  for (i = 0; i<len; i++)
  {
  
         for(int ii = 7; ii>=0; ii--)
         {
  
               DoInvert = (0x01 & (BitString[i]>>ii)) ^ crc>>7;         // XOR required?
  
               crc = crc ^ (DoInvert<<3 | DoInvert << 4);
               crc = crc <<1;
               crc &= ~0x01;
               crc |= DoInvert;
         }
  }
  return(crc);

}


// PIC12F1572
// https://pic-microcontroller.com/interfacing-pic16f877a-dht22am2302-rht03-sensor-using-ccs-pic-c/
// http://ww1.microchip.com/downloads/en/devicedoc/40001723d.pdf

  
void start_signal()
{
  Time_out = 0;
  DIR_AM2302 = PORT_OUT;              // Configure connection pin as output
  PIN_AM2302 = 0;                        // Connection pin output low
  delay_ms(2);  // data sheet says "at least 1ms"
  DIR_AM2302 = PORT_IN;              // Configure connection pin as input -> pull up signal goes High
  PIN_AM2302 = 1;                        // Connection pin output high
  delay_us(30);

}

short check_response()
{
  delay_us(40);
  PIN_DATA = 0;
  if(!PIN_AM2302)
  {                     // Read and test if connection pin is low
    delay_us(70);
    if(PIN_AM2302)
    {                    // Read and test if connection pin is high
      delay_us(50);
      return 1;
    }
  }
  return 0;
}

unsigned char Read_Data()
{
  unsigned char i, k, _data = 0;     // k is used to count 1 bit reading duration
  if(Time_out)
    return 0; 
    
  for(i = 0; i < 8; i++)
  {
    k = 0;
    while(!PIN_AM2302)
    {                          // Wait until pin goes high
      k++;
      if (k > 100)
      {
        Time_out = 1;
        break;
      }
      delay_us(1);
    }
    delay_us(30);
    if(!PIN_AM2302)
    {
      //bit_clear(_data, (7 - i));               // Clear bit (7 - i)
      _data &= ~(1 << (7 - i)); // Clear bit (7 - i)
    }
    else
    {
      //bit_set(_data, (7 - i));                 // Set bit (7 - i)
       _data |= (1 << (7 - i)); // Set bit (7 - i)
      while(PIN_AM2302)
      {                         // Wait until pin goes low
        k++;
        if (k > 100)
        {
          Time_out = 1;
          break;
        }
        delay_us(1);
      }
    }
  }
  return _data;
}


void delay_ms_var(unsigned int milliseconds)
 {
   while(milliseconds > 0)
   {
       __delay_ms(1);
      milliseconds--;
    }
 }
void ErrorLED(int volteblink, unsigned int time_ms)
{
    // solo se non dbg!!
    // blink data line
        TRISAbits.TRISA0 = PORT_OUT;
        
        for(int i = 0; i<volteblink;i++)
        {
            PORTAbits.RA0 = 1;
            delay_ms_var(time_ms);
            PORTAbits.RA0 = 0;
            if((i+1)<volteblink) // non fa pausa se é ultimo, tanto poi rimane spento
                delay_ms_var(time_ms);
        }
        
}

void main(void)
{

    // Init Oscillator
    OSCCONbits.SCS  = 0b10; // internal oscillator
    OSCCONbits.IRCF = 0b1111;  // 500kHz Fosc ?? basterà? OSCCONbits.IRCF = 0b1111 -> 16Mhz

    // Init Port
    PIN_AM2302 = 1;
    PIN_DATA   = 0;
    PIN_CS     = 1;

    DIR_AM2302       = PORT_IN; // InOut AM2302, start input
    TRISAbits.TRISA5 = PORT_OUT; // output DATA WS
    TRISAbits.TRISA4 = PORT_IN; // input  CS   WS

    
    ANSELA = 0x00; // all digital
    
    OPTION_REGbits.nWPUEN = 0;
    WPUAbits.WPUA4 = 1; // CS input with Wake pullup

    ODCONAbits.ODA2 = 1; // open drain ouput AM2302
    ODCONAbits.ODA5 = 0; // NON open drain ouput DATA


    // Init Timer 1
    //T1CONbits.T1CKPS = 0b11; // prescaler 8
    //T1CONbits.TMR1CS = 0b11; // LFINTOSC (31kHz clock source) -> 258us inc
    //T1CONbits.nT1SYNC = 1; // not sync
    //TMR1 = TIM1_RELOAD_VAL;


    unsigned char T_byte1, T_byte2, RH_byte1, RH_byte2, CheckSum ;
    unsigned char Data[6];
    unsigned int CountErrors = 0;
    
   /* while(1)
    {
        PIN_DATA = 1;
        delay_ms(100);
        PIN_DATA = 0;
        delay_ms(100);
    }
    */
    
    while(1)
    {
        PORTAbits.RA3 = 1;
        /******************* READ SENSOR *************************/
        start_signal();
        if(check_response())
        {                    // If there is response from sensor
          RH_byte1 = Read_Data();                 // read RH byte1
          RH_byte2 = Read_Data();                 // read RH byte2
          T_byte1  = Read_Data();                  // read T byte1
          T_byte2  = Read_Data();                  // read T byte2
          CheckSum = Read_Data();                 // read checksum
          if(Time_out)
          {                           // If reading takes long time
//              lcd_putc("Time out!");
              CountErrors ++;
              ErrorLED(5,100);
          }
          else
          {
            if(CheckSum == ((RH_byte1 + RH_byte2 + T_byte1 + T_byte2) & 0xFF))
            {
              RH = RH_byte1;
              RH = (RH << 8) | RH_byte2;     // in 1/10 %
              Temp = T_byte1;
              Temp = (Temp << 8) | T_byte2;  // in 1/10 °C


              int temp = Temp + 400 -12; //val = 10*t + 400; 
              Data[0] = (temp>>8)&0xFF;
              Data[1] = (temp)&0xFF;
              Data[2] = RH/10; // value = % di umidià
              Data[5] = MakeCRC(&Data[0],5);
  
              ErrorLED(1,100); // singolo lettura ok
            }
            else
            {
                CountErrors ++;
//              lcd_putc("Checksum Error!");
                ErrorLED(3,100);         
            }
          }
        
        }
        else
        {
            ErrorLED(4,100);
        }
        
        //delay_ms(100);
        /******************* IF CS SEND VALUE, every TM read sensor Again *************************/
        int k=0;
        while(1)
        {                         // Wait until pin goes low
          k++;
          if (k >4000) // timeout 4s, afterthat read new value
          {
            break; // goto do new read...
          }
          delay_ms(1);
          
          // check CS
          if(PIN_CS == 0)
          {
            // send data
            delay_ms(8);              // wait for 1ms
            
            for(int dato=0; dato<6; dato ++)
            {
              for(int i=7; i>=0;i--)
              {
                PIN_DATA = 1;
                if( ((Data[dato]>>i) & 0x01) == 0x01)
                {
                  // tx 1 (lungo corto)
                   delay_us(500);              // wait for 500us
                }
                else
                {
                  // tx 0 (lungo lungo)
                   delay_us(1500);              // wait for 1500us
                }
                
                
                PIN_DATA = 0;
                delay_us(1000);              // wait for 1ms
                
              }
            }
            k += 150; //durata send msg
            
            // wait cs goes to 1
            ErrorLED(2,100); // doppio invio valore ok
            
            for(int kk=0; (kk<1000)&& (PIN_CS==0) ;kk++)
            {
                delay_ms(1);
                k++;
            }
          }
          
          
        }

  

    }
}





