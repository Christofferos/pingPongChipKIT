/* time4io

  Written by Kristopher Werlinder and Amanda Strömdahl 2019-09-24
*/

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */

/*
  Extracts data for SW1, SW2, SW3 and SW4 from PORTD.
*/
int getsw(void)
{
  return ((PORTD >> 8) & 0xf);
}

/*
  Extracts data for BTN1, BTN2, BTN3 and BTN4 from PORTD.
*/
int getbtns(void)
{
  return ((((PORTD >> 5) & 0x7) << 1) | ((PORTF >> 1) & 0x1));
}
