
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

// display = a matrix for the pixel dimensions is 32 pixels in height (y) and 128 pixels in width (x)
uint8_t display[32][128];
// oled_display = a matrix that can be interpreted by the hardware  
uint8_t oled_display[512];

// Variables to keep track of navigation between different screens
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

// AI difficulty variables
float aiPadelSpeed = 0.75;

// Variables that are needed for (quicksleeps/delays) in the start and end of every screen
int startOfMenu = 1;
int startOfModes = 0;
int startOfHighscores = 0;
int startOfGame = 1;
int endOfGame = 0;

// Highscore specific variables
int survivalScore = 0;
int highScoreList[3] = {0, 0, 0};
  // Highscore int to char array
  char snum1[10] = " #1 ";
  char snum2[10] = " #2 ";
  char snum3[10] = " #3 ";
  char buf1[sizeof(int)*3+2];
  char buf2[sizeof(int)*3+2];
  char buf3[sizeof(int)*3+2];

  char concatenatedScore[16];
  char spaceTheScore[7] = "       ";


// Padel and player variables
float padelHeight = 7;
float padelWidth = 4;
float padelSpeed = 1;
int leds = 0xf;

float xPosPadel1 = 0;
float yPosPadel1 = 32/2 - 5;
int player1Score = 0;

float xPosPadel2 = 127 - 4;
float yPosPadel2 = 32/2 - 5;
int player2Score = 0;

// Ball variables
float ballSize = 3;
float ballSpeedX = 1;
float ballSpeedY = 0;
float xPosBall = 128/2- 2;
float yPosBall = 16;
float maxBallSpeedX = 3;

float randomNumber = 0;

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

void labinit(void)
{
  volatile int *trise = (volatile int *)0xbf886100;
  TRISECLR = 0xff;  // Set as output (LED:S)
  TRISDSET = 0xfe0; // Set as input (BTN 2-4, SW 1-4)
  TRISFSET = 0x2; // Set as input (BTN 1)
  PORTE = 0x0;        // LED:s
}


void getRandom() {
  randomNumber = TMR2 % 5;
  randomNumber /= 10;
  randomNumber += 0.5;
  randomNumber *= getRandomSign();
}

int getRandomSign() {
  if(TMR2 % 10 < 5) {
    return -1;
  }
  return 1;
}

void ledControl() {
  PORTE = leds;
}

// Set array slots to ones or zeros in the 2D array.
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

// Translate the 2D array into the 1D array with 512 slots. 
void translateToImage()
{
  int page, column, row, c, k;
  uint8_t powerOfTwo = 1; // Interval: 2^0, 2^1 ... to ... 2^7.  
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
      // Display score in survival mode
      if (survivalMode && page == 0) {
        if (column % 8 == 0) {
          c = textbuffer[page][column/8];
        }
        if (!(c & 0x80)) {
          oledNumber |= font[c*8 + column%8];
        }
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

void resetGame() {
  xPosPadel1 = 0;
  yPosPadel1 = 32/2 - padelHeight/2;

  xPosPadel2 = 127 - padelWidth;
  yPosPadel2 = 32/2 - padelHeight/2;

  xPosBall = 128/2 - ballSize/2;
  yPosBall = 16;

  clearDisplay();
  setPixelArray(xPosPadel1, yPosPadel1, padelWidth, padelHeight);
  setPixelArray(xPosPadel2, yPosPadel2, padelWidth, padelHeight);
  setPixelArray(xPosBall, yPosBall, ballSize, ballSize);
  translateToImage();
  display_image(0, oled_display);

  startOfGame = 1;
}

int shiftDir = 1; // Höger
void lightshow(int winner) {
  PORTE = 0x8;
  int count = 0;
  if (!winner) { // Dark side
    PORTE <<= 4;
    while (count < 48) {
      if(PORTE & 0x10) {
        shiftDir = 0;
      }
      else if(PORTE & 0x80) {
        shiftDir = 1;
      }
      if(shiftDir) {
        PORTE >>= 1;
      }
      else if (!shiftDir) {
        PORTE <<= 1;
      }
      count++;
      quicksleep(1 << 19);
    }
  } else { // Light side
    while (count < 48) {
      if(PORTE & 0x1) {
        shiftDir = 0;
      }
      else if(PORTE & 0x8) {
        shiftDir = 1;
      }
      if(shiftDir) {
        PORTE >>= 1;
      }
      else if (!shiftDir) {
        PORTE <<= 1;
      }
      count++;
      quicksleep(1 << 19);
    }
  }
}

void endGame(void) {
  if (!leds) {
    display_string(0, "");
    display_string(2, "");
    display_string(3, "");
    display_string(1, "Dark Side wins!");
    display_update();
    lightshow(0);

  } else if (leds == 0xff) {
    display_string(0, "");
    display_string(3, "");
    if (survivalMode) {
      display_string(1, "   Your score:");
      if (survivalScore >= highScoreList[0]) {
        strcat(concatenatedScore, " :D");
      } else if (survivalScore >= highScoreList[1] && survivalScore < highScoreList[0]) {
        strcat(concatenatedScore, " :)");
      } else if (survivalScore >= highScoreList[2] && survivalScore < highScoreList[1]) {
        strcat(concatenatedScore, " :|");
      } else {
        strcat(concatenatedScore, " >:(");
      }
      display_string(2, concatenatedScore);
    } else {
      display_string(2, "");
      display_string(1, "Light Side wins!");
    }
    display_update();
    lightshow(1);

  }
  // Update the highscore system:
  if(survivalMode == 1) {
    if(highScoreList[0] < survivalScore) {
      if(highScoreList[0] != 0) {
        strcpy(snum1, " #1 ");

        strcpy(snum2, " #2 ");
        snprintf(buf2, sizeof buf2, "%d", highScoreList[0]);
        strcat(snum2, buf2);

        strcpy(snum3, " #3 ");
        snprintf(buf3, sizeof buf3, "%d", highScoreList[1]);
        strcat(snum3, buf3);

        highScoreList[2] = highScoreList[1];
        highScoreList[1] = highScoreList[0];
      }
      snprintf(buf1, sizeof buf1, "%d", survivalScore);
      strcat(snum1, buf1);
      highScoreList[0] = survivalScore;
    } else if (highScoreList[1] < survivalScore) {
      if(highScoreList[1] != 0) {
        strcpy(snum2, " #2 ");

        strcpy(snum3, " #3 ");
        snprintf(buf3, sizeof buf3, "%d", highScoreList[1]);
        strcat(snum3, buf3);

        highScoreList[2] = highScoreList[1];
      }
      snprintf(buf2, sizeof buf2, "%d", survivalScore);
      strcat(snum2, buf2);
      highScoreList[1] = survivalScore;
    } else if (highScoreList[2] < survivalScore) {
      if(highScoreList[2] != 0) {
        strcpy(snum3, " #3 ");
      }
      snprintf(buf3, sizeof buf3, "%d", survivalScore);
      strcat(snum3, buf3);
      highScoreList[2] = survivalScore;
    }
    survivalScore = 0;
  }
  // SET VARIABLES to correct values
  inGame = 0;
  inMenu = 1;
  startOfMenu = 1;
  startOfGame = 1;
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

void clearStringDisplay() {
  display_string(0, "              ");
  display_string(1, "              ");
  display_string(2, "              ");
  display_string(3, "              ");
}

void padelCollide(void)
{
  // PADEL 1
  if ( ballSpeedX < 0 && (xPosBall >= xPosPadel1) && (xPosBall <= (xPosPadel1 + padelWidth)) && ((yPosBall + ballSize) > yPosPadel1) && (yPosBall < (yPosPadel1 + padelHeight))) {
    ballSpeedX *= -1;
    if(ballSpeedX < maxBallSpeedX) {
      ballSpeedX += 0.5;
    }
    if ((yPosBall + ballSize/2) < yPosPadel1 + padelHeight/2 && ballSpeedY > -1.5) {
      if(ballSpeedX < -2) {
        ballSpeedY -= 0.55;
      } else {
        ballSpeedY -= 0.35;
      }
    } else if ((yPosBall + ballSize/2) > yPosPadel1 + 2*padelHeight/2 && ballSpeedY < 1.5) {
      if(ballSpeedX < -2) {
        ballSpeedY += 0.55;
      } else {
        ballSpeedY += 0.35;
      }
    }
    // Survival score increment
    if(survivalMode) {
      survivalScore++;
    }
  }
  // PADEL 2
  else if ( ballSpeedX > 0 && ((xPosBall + ballSize) >= xPosPadel2) && ((xPosBall + ballSize) <= xPosPadel2 + padelWidth) && (((yPosBall + ballSize) > yPosPadel2)) && (yPosBall < (yPosPadel2 + padelHeight))) {
    ballSpeedX *= -1;
    if(ballSpeedX > -maxBallSpeedX) {
      ballSpeedX -= 0.5;
    }
    if ((yPosBall + ballSize/2) < yPosPadel2 + padelHeight/2 && ballSpeedY > -1.5) {
      if(ballSpeedX > 2) {
        ballSpeedY -= 0.55;
      } else {
        ballSpeedY -= 0.35;
      }
    } else if ((yPosBall + ballSize/2) > yPosPadel2 + 2*padelHeight/2 && ballSpeedY < 1.5) {
      if(ballSpeedX > 2) {
        ballSpeedY += 0.55;
      } else {
        ballSpeedY += 0.35;
      }
    }
  }

  if ((xPosBall + ballSize) < 0 || xPosBall > 128) {
    quicksleep(1 << 16);
    goal();
  }
}

void playerMovement(void)
{
  if (defaultMode && (getbtns() & 0x1) && (yPosPadel2 < (31 - padelHeight)))
  {
    yPosPadel2 += padelSpeed;
  }
  else if (defaultMode && (getbtns() & 0x2) && (yPosPadel2 > 0))
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
      startOfGame = 1;
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

void aiDifficulty() {
  if(aiMode) {
    if(getsw() & 0x1) {
      aiPadelSpeed = 0.9;
    } else if (getsw() & 0x2) {
      aiPadelSpeed = 1.05;
      maxBallSpeedX = 3.5;
    } else if (getsw() & 0x4) {
      aiPadelSpeed = 1.20;
      maxBallSpeedX = 4;
    } else {
      aiPadelSpeed = 0.75;
    }
  }
}

void quitGameOnCommand() {
  if(inGame && getsw() & 0x8) {
    resetGame();
    endGame();
  }
}

/* This function is called repetitively from the main program */
void labwork()
{
  quicksleep(1 << 15);
  if (inGame ) {
    clearStringDisplay();
    setPixelArray(xPosPadel1, yPosPadel1, padelWidth, padelHeight);
    setPixelArray(xPosPadel2, yPosPadel2, padelWidth, padelHeight);
    if (survivalMode) {
      if (yPosBall < padelHeight / 2) {
        yPosPadel2 = 0;
      } else if ((yPosBall + ballSize) > (31 - padelHeight / 2)) {
        yPosPadel2 = 31 - padelHeight;
      } else {
        yPosPadel2 = yPosBall + ballSize / 2 - padelHeight / 2;
      }
      strcpy(concatenatedScore, spaceTheScore);
      strcat(concatenatedScore, itoaconv(survivalScore));
      display_string(0, concatenatedScore);

    }
    else if (aiMode) {
        aiDifficulty();
       if (xPosBall > 127/2) {
          if((yPosPadel2+padelHeight/2) < (yPosBall+ballSize/2) && yPosPadel2 < (31 - padelHeight)) {
            yPosPadel2 += aiPadelSpeed;
          } else if((yPosPadel2+padelHeight/2) > (yPosBall+ballSize/2) && yPosPadel2 > 0) {
            yPosPadel2 -= aiPadelSpeed;
          }
      }
      else {
        if ((yPosPadel2 + padelHeight / 2) < 14.8) {
          yPosPadel2 += 0.1;
        } else if ((yPosPadel2 + padelHeight / 2) > 15.2) {
          yPosPadel2 -= 0.1;
        }
      }
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
    quitGameOnCommand();
    if (startOfGame) {
      quicksleep(1 << 23);
      startOfGame = 0;
      getRandom();
      ballSpeedY = randomNumber;
      ballSpeedX = getRandomSign();
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
    	display_string(0, snum1); // Convert elements to string!
      display_string(1, snum2);
    	display_string(2, snum3);
      display_string(3, "> Back to menu");
      display_update();
      startOfHighscores = 0;
    }
  }
}
