/* mipslabwork.c*/
/* mipslabwork.c

   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall
   Modified by Kristopher Werlinder and Amanda Strömdahl 2019-09-30

   This file modified 2017-04-31 by Ture Teknolog

   For copyright and licensing, see file COPYING */

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */

int xPos = 1;
int xSpeed = 1;
int yPos = 1;
int ySpeed = 1;
int delayCounter = 0;

int mytime = 0x5957;
int timeoutcount = 0;
int tickCounter = 0;
char textstring[] = "text, more text, and even more text!";

/* Interrupt Service Routine */
void user_isr( void ) {
    timeoutcount++;
    if (timeoutcount == 10) { // timeoutcount wraps at 10
      IFSCLR(0) = 0x100;
      timeoutcount = 0;
      time2string( textstring, mytime );
      display_string( 3, textstring );
      display_update();
      tick( &mytime );
   }
   if (getsw() & 0x1) {
     IFSCLR(0) = 0x80;
     tickCounter += 0x1;
     PORTE = tickCounter;
   }
}

/* Lab-specific initialization goes here */
void labinit( void )
{
  volatile int* trise = (volatile int*) 0xbf886100;
  *trise &= ~ 0xff; // (1c) Set as output
  TRISDSET = 0xfe0; // (1e) Set as input
  PORTE = 0; // LED:s


  T2CONSET = 0x8070; // Sets prescaler to 1:256
                  // Clock rate divider = prescaler = T2CON
  TMR2 = 0;       // Resets clock
  PR2 = 80000000/256/10; // PR2 = Time out period

  // SW1
  IPCSET(1) = 0xf800000;

  // Timer 2
  IPCSET(2) = 0x1f;

  IECSET(0) = 0x180;
  //enable_interrupt();
}

/* This function is called repetitively from the main program */
void labwork( void ) {
  delayCounter++;
  const uint8_t const padel[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  display_image(50, padel); // First parameter: position on x-axis, integer between 0 and 100

  if (delayCounter == 100) {

    delayCounter=0;
    if(xPos== 100 || xPos == 0) {
      xSpeed *= -1;
    }
    xPos += xSpeed;

    display_update();
  }
}
