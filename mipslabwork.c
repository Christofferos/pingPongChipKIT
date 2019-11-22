
/* mipslabwork.c*/
/* mipslabwork.c

   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall
   Modified by Kristopher Werlinder and Amanda Strömdahl 2019-09-30

   This file modified 2017-04-31 by Ture Teknolog

   For copyright and licensing, see file COPYING */

#include <stdint.h>  /* Declarations of uint_32 and the like */
#include <pic32mx.h> /* Declarations of system-specific addresses etc */
#include "mipslab.h" /* Declatations for these labs */
#include <stdio.h>
#include <stdlib.h>

uint8_t display[32][128];
uint8_t oled_display[512];
int inMenu = 1;
int inGame = 0;
int inModes = 0;
int inHighScores = 0;
int menuCursor = 0;
int modesCursor = 0;

// Modes
int defaultMode = 1;
int aiMode = 0;
int survivalMode = 0;

int startOfMenu = 1;
int startOfModes = 0;
int startOfHighscores = 0;
int startOfGame = 1;
int endOfGame = 0;
int highScoreList[3];

// Padel and player variables
int padelHeight = 10;
int padelWidth = 4;
int padelSpeed = 1;
int leds = 0xf;

int xPosPadel1 = 0;
int yPosPadel1 = 32/2 - 5;
int player1Score = 0;

int xPosPadel2 = 124;
int yPosPadel2 = 32/2 - 5;
int player2Score = 0;

// Ball variables
int ballSize = 4;
int ballSpeedX = 1;
int ballSpeedY = 1;
int xPosBall = 128/2- 2;
int yPosBall = 16;

int mytime = 0x5957;
int timeoutcount = 0;
int tickCounter = 0;

/* Interrupt Service Routine */
void user_isr(void)
{
  if (IFS(0) & 0x100)
  {
    timeoutcount++;
    IFSCLR(0) = 0x100;
    if (timeoutcount == 10)
    { // timeoutcount wraps at 10
      timeoutcount = 0;
    }
  }
}

/* Lab-specific initialization goes here */
void labinit(void)
{
  volatile int *trise = (volatile int *)0xbf886100;
  TRISECLR = 0xff;  // Set as output (LED:S)
  TRISDSET = 0xfe0; // Set as input (BTN 2-4, SW 1-4)
  TRISFSET = 0x2; // Set as input (BTN 1)
  PORTE = 0x0;        // LED:s

  T2CONSET = 0x8070;         // Sets prescaler to 1:256
                             // Clock rate divider = prescaler = T2CON
  TMR2 = 0;                  // Resets clock
  PR2 = 80000000 / 256 / 10; // PR2 = Time out period

  // SW1
  IPCSET(1) = 0xf800000;

  // Timer 2
  IPCSET(2) = 0x1f;

  IECSET(0) = 0x180;
  //enable_interrupt();
}

void ledControl() {
  PORTE = leds;
}

void setPixelArray(int xPos, int yPos, int xlen, int ylen)
{
  int row, column;
  for (row = 0; row < 32; row++)
  { // y-axis
    for (column = 0; column < 128; column++)
    { // x-axis
      if (row >= yPos && row <= (yPos + ylen) && column >= xPos && column <= (xPos+ xlen))
      {
        display[row][column] = 1;
      }
    }
  }
}

void translateToImage()
{
  int page, column, row;
  uint8_t powerOfTwo = 1; // Interval: 2^0, 2^1 ... to ... 2^7
  uint8_t oledNumber = 0;

  for (page = 0; page < 4; page++)
  {
    for (column = 0; column < 128; column++)
    {
      powerOfTwo = 1;
      oledNumber = 0;

      for (row = 0; row < 8; row++)
      {
        if (display[8 * page + row][column])
        {
          oledNumber |= powerOfTwo;
        }
        powerOfTwo <<= 1;
      }
      oled_display[column + page * 128] = oledNumber;
    }
  }
}

void clearDisplay()
{
  /* CLEARS ALL elements in arrays and sets 0 on them.
    memset(display, 0, sizeof(display));
    memset(oled_display, 0, sizeof(oled_display));
  */
  int row, column, i;
  for (row = 0; row < 32; row++)
  {
    for (column = 0; column < 128; column++)
    {
      display[row][column] = 0;
    }
  }
  for (i = 0; i < 512; i++)
  {
    oled_display[i] = 0;
  }
}

/* Game Logic - START */
void updateGame(void)
{
  xPosBall += ballSpeedX;
  yPosBall += ballSpeedY;
  // Roof and floor ball bounce
  if (yPosBall < 0 || yPosBall > (31 - ballSize))
  {
    ballSpeedY *= -1;
  }
}

void padelCollide(void)
{
  // ballSpeedX > 0 kan tas bort från if-satsen när bollen inte längre kan studsa mot skärmens kortsidor
  if ((ballSpeedX < 0) && (xPosBall == (xPosPadel1 + padelWidth)) && ((yPosBall + ballSize) > yPosPadel1) && (yPosBall < (yPosPadel1 + padelHeight))) {
    ballSpeedX *= -1;
  }
  // ballSpeedX > 0 kan tas bort från if-satsen när bollen inte längre kan studsa mot skärmens kortsidor
  if ((ballSpeedX > 0) && ((xPosBall + ballSize) == xPosPadel2) && (((yPosBall + ballSize) > yPosPadel2)) && ((yPosBall < (yPosPadel2 + padelHeight)))) {
    ballSpeedX *= -1;
  }
  if ((xPosBall + ballSize) < 0 || xPosBall > 128) {
    quicksleep(1 << 16);
    goal();
  }
}

void goal() {
  // Check for goals
  if (xPosBall < 0)
  {
    leds = ((leds << 1) | 0x1);
    player2Score++;
  }
  else if (xPosBall > (128 - ballSize))
  {
    leds >>= 1;
    player1Score++;
  }
  resetGame();
  if (leds == 0x0 || leds == 0xff) {
    endGame();
  }
}

void resetGame() {
  xPosPadel1 = 0;
  yPosPadel1 = 32/2 - padelHeight/2;

  xPosPadel2 = 128 - padelWidth;
  yPosPadel2 = 32/2 - padelHeight/2;

  xPosBall = 128/2 - ballSize/2;
  yPosBall = 16;
  ballSpeedX *= -1;

  clearDisplay();
  setPixelArray(xPosPadel1, yPosPadel1, padelWidth, padelHeight);
  setPixelArray(xPosPadel2, yPosPadel2, padelWidth, padelHeight);
  setPixelArray(xPosBall, yPosBall, ballSize, ballSize);
  translateToImage();
  display_image(0, oled_display);

  startOfGame = 1;
}

void endGame(void) {
  if (!leds) {
    display_string(0, "");
    display_string(1, "");
    display_string(2, "");
    display_string(3, "");
    display_string(1, "Dark Side wins!");
    display_update();

    quicksleep(1 << 18);
    inGame = 0;
    inMenu = 1;
    startOfMenu = 1;
  } else if (leds == 0xff) {
    display_string(0, "");
    display_string(1, "");
    display_string(2, "");
    display_string(3, "");
    display_string(1, "Light Side wins!");
    display_update();

    quicksleep(1 << 18);
    inGame = 0;
    inMenu = 1;
    startOfMenu = 1;
  }
}

void playerMovement(void)
{
  if ((getbtns() & 0x1) && (yPosPadel2 < (31 - padelHeight)))
  {
    yPosPadel2 += padelSpeed;
  }
  else if ((getbtns() & 0x2) && (yPosPadel2 > 0))
  {
    yPosPadel2 -= padelSpeed;
  }
  if ((getbtns() & 0x4) && (yPosPadel1 < (31 - padelHeight)))
  {
    yPosPadel1 += padelSpeed;
  }
  else if ((getbtns() & 0x8) && (yPosPadel1 > 0))
  {
    yPosPadel1 -= padelSpeed;
  }
}
/* ^Game logic - END */

void moveMenuCursor(void) {
  display_string(0, "  PONG!");
  if (menuCursor == 0) {
    display_string(1, "> Start");
    display_string(2, "  Modes");
    display_string(3, "  High Scores");
  } else if (menuCursor == 1) {
    display_string(1, "  Start");
    display_string(2, "> Modes");
    display_string(3, "  High Scores");
  } else if (menuCursor == 2) {
    display_string(1, "  Start");
    display_string(2, "  Modes");
    display_string(3, "> High Scores");
  }
  display_update();

  /* Variabler kan användas för att nå högre precition: screenWidth/3, screenHeight/3 */
}

void menu(void) {
  quicksleep(1 << 20);
  if (getbtns() & 0x1 && menuCursor == 0) {
      inMenu = 0;
      menuCursor = 0;
      inGame = 1;
      leds = 0xf;
  }
  else if (getbtns() & 0x1 && menuCursor == 1) {
    inMenu = 0;
    menuCursor = 0;
    inModes = 1;
    startOfModes = 1;
  }
  else if (getbtns() & 0x1 && menuCursor == 2) {
    inMenu = 0;
    menuCursor = 0;
    inHighScores = 1;
    startOfHighscores = 1;
  }
  else if ((getbtns() & 0x2) && menuCursor != 2) {
    menuCursor++;
    moveMenuCursor();
  }
  else if ((getbtns() & 0x4) && menuCursor != 0) {
    menuCursor--;
    moveMenuCursor();
  }
}

void highscores() {
  quicksleep(1 << 20);
  if (getbtns() & 0x1) {
      inMenu = 1;
      inHighScores = 0;
      startOfMenu = 1;
  }
}

void moveModesCursor() {
  display_string(0, "  MODES");
  if (modesCursor == 0) {
    display_string(1, "> Default");
    display_string(2, "  Human vs AI");
    display_string(3, "  Survival");
  } else if (modesCursor == 1) {
    display_string(1, "  Default");
    display_string(2, "> Human vs AI");
    display_string(3, "  Survival");
  } else if (modesCursor == 2) {
    display_string(1, "  Default");
    display_string(2, "  Human vs AI");
    display_string(3, "> Survival");
  }
  display_update();
}

void modes() {
  quicksleep(1 << 20);
  if (getbtns() & 0x1 && modesCursor == 0) {
      inMenu = 1;
      inModes = 0;
      modesCursor = 0;
      defaultMode = 1;
      aiMode = 0;
      survivalMode = 0;
      startOfMenu = 1;
  }
  else if (getbtns() & 0x1 && modesCursor == 1) {
    inMenu = 1;
    inModes = 0;
    modesCursor = 0;
    defaultMode = 0;
    aiMode = 1;
    survivalMode = 0;
    startOfMenu = 1;
  }
  else if (getbtns() & 0x1 && modesCursor == 2) {
    inMenu = 1;
    inModes = 0;
    modesCursor = 0;
    defaultMode = 0;
    aiMode = 0;
    survivalMode = 1;
    startOfMenu = 1;
  }
  else if ((getbtns() & 0x2) && modesCursor != 2) {
    modesCursor++;
    moveModesCursor();
  }
  else if ((getbtns() & 0x4) && modesCursor != 0) {
    modesCursor--;
    moveModesCursor();
  }
}

/*char * toArray(int number) //Copied function
    {
        int n = log10(number) + 1;
        int i;
      char *numberArray = calloc(n, sizeof(char));
        for ( i = 0; i < n; ++i, number /= 10 )
        {
            numberArray[i] = number % 10;
        }
        return numberArray;
    }*/

/* This function is called repetitively from the main program */
void labwork(void)
{
  quicksleep(1 << 15);
  if (inGame) {
    setPixelArray(xPosPadel1, yPosPadel1, padelWidth, padelHeight);
    if (defaultMode) {
      setPixelArray(xPosPadel2, yPosPadel2, padelWidth, padelHeight);
    }
    setPixelArray(xPosBall, yPosBall, ballSize, ballSize);
    translateToImage();
    if (!endOfGame) {
      display_image(0, oled_display);
    }
    playerMovement();
    padelCollide();
    updateGame();
    ledControl();
    if (startOfGame) {
      quicksleep(1 << 23);
      startOfGame = 0;
    }
    clearDisplay();
  } else if (inMenu) {
    menu();
    if (startOfMenu) {
      PORTE = 0x0;
      display_string(0, "  PONG!");
    	display_string(1, "> Start");
    	display_string(2, "  Modes");
    	display_string(3, "  High Scores");
    	display_update();
      startOfMenu = 0;
    }

  } else if (inModes) {
    modes();
    if (startOfModes) {
      display_string(0, "  MODES");
    	display_string(1, "> Default");
    	display_string(2, "  Human vs AI");
    	display_string(3, "  Survival");
    	display_update();
      startOfModes = 0;
    }
  }
  else if (inHighScores) {
    highscores();
    if (startOfHighscores) {
    	display_string(0, " #1 " + highScoreList[0]); // Convert elements to string!
    	display_string(1, " #2 " + highScoreList[1]);
    	display_string(2, " #3 " + highScoreList[2]);
      display_string(3, "> Back to menu");
      display_update();
      startOfHighscores = 0;
    }
  }
}
