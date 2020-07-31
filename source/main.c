#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <gcmodplay.h>
#include <aesndlib.h>

#include "addicti_mod.h"
#include "ancient_days__mod.h"
#include "diginnov_mod.h"
#include "madbit___melody_mod.h"
#include "morphed_dreams_s_mod.h"
#include "upfront_mod.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static MODPlay play;

// 0: Welcome Screen
// 1: Game
// 2: Game Over
s8 gameState = 0;

u16 numGrid[4][4] = {
	{0,0,0,0},
	{0,0,0,0},
	{0,0,0,0},
	{0,0,0,0},
};

u16 prevNumGrid[4][4] = {
	{0,0,0,0},
	{0,0,0,0},
	{0,0,0,0},
	{0,0,0,0},
};

s8 songIndex = 0;

void initScreen() {
	printf("\e[1;1H\e[2J");
	printf("\x1b[%d;%dH", 2, 4);
	printf("2048 Wii");
}

void generateNewTile() {
	s8 gridVals[16];
	s8 numEmptySpaces = 0;
	for (s8 i=0; i<16; i++) {
		if (numGrid[i%4][i/4] == 0) {
			gridVals[numEmptySpaces] = i;
			numEmptySpaces++;
		}
	}
	if (numEmptySpaces > 0) {
		s8 tileNum = gridVals[rand()%numEmptySpaces];
		if (rand()%10 < 9)
			numGrid[tileNum%4][tileNum/4] = 2;
		else
			numGrid[tileNum%4][tileNum/4] = 4;
	}
}

s8 checkForWin() {
	for (s8 i=0; i<4; i++) {
		for (s8 j=0; j<4; j++) {
			if (numGrid[i][j] >= 2048) {
				printf("\x1b[%d;%dH", 30, 14);
				printf("YOU WIN!!");
				gameState = 2;
				return 1;
			}
		}
	}
	return 0;
}

s8 moveIsPossible(s8 i, s8 j) {
	if (i > 0 && (numGrid[i-1][j] == 0 || numGrid[i-1][j] == numGrid[i][j]))
		return 1;
	if (i < 3 && (numGrid[i+1][j] == 0 || numGrid[i+1][j] == numGrid[i][j]))
		return 1;
	if (j > 0 && (numGrid[i][j-1] == 0 || numGrid[i][j-1] == numGrid[i][j]))
		return 1;
	if (j < 3 && (numGrid[i][j+1] == 0 || numGrid[i][j+1] == numGrid[i][j]))
		return 1;
	return 0;
}

s8 checkForLoss() {
	for (s8 i=0; i<4; i++) {
		for (s8 j=0; j<4; j++) {
			if (moveIsPossible(i,j)) {
				return 0;
			}
		}
	}
	printf("\x1b[%d;%dH", 30, 13);
	printf("Game Over");
	gameState = 2;
	return 1;
}

void printNums() {
	u16 currNum;
	for (s8 i=0; i<4; i++) {
		for (s8 j=0; j<4; j++) {
			currNum = numGrid[i][j];
			if (currNum == 0)
				continue;
			printf("\x1b[%d;%dH", 7+i*6, 5+7*j);
			if (currNum < 1000)
				printf("%4d", numGrid[i][j]);
			else if (currNum < 100000)
				printf("%5d", numGrid[i][j]);
			else
				printf("%6d", numGrid[i][j]);
		}
	}
}

void printGrid() {
	printf("\x1b[%d;%dH", 4, 0);
	printf("    +------+------+------+------+\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    +------+------+------+------+\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    +------+------+------+------+\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    +------+------+------+------+\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    |      |      |      |      |\n");
	printf("    +------+------+------+------+\n");
	printNums();
}

void initNumGrid() {
	for (s8 i=0; i<4; i++) {
		for (s8 j=0; j<4; j++) {
			numGrid[i][j] = 0;
		}
	}
	generateNewTile();
	generateNewTile();
	printGrid();
}

// dir = 0 (up), 1 (down), 2 (left), 3 (right)
s8 moveTile(s8 src_i, s8 src_j, s8 dir, s8 canMerge) {
	s8 dest_i;
	s8 dest_j;
	switch (dir) {
		case 0:
			if (src_i == 0)
				return 0;
			dest_i = src_i - 1;
			dest_j = src_j;
			break;
		case 1:
			if (src_i == 3)
				return 0;
			dest_i = src_i + 1;
			dest_j = src_j;
			break;
		case 2:
			if (src_j == 0)
				return 0;
			dest_i = src_i;
			dest_j = src_j - 1;
			break;
		default:
			if (src_j == 3)
				return 0;
			dest_i = src_i;
			dest_j = src_j + 1;
			break;
	}
	if (numGrid[dest_i][dest_j] == 0) {
		numGrid[dest_i][dest_j] = numGrid[src_i][src_j];
		numGrid[src_i][src_j] = 0;
		s8 returnVal = moveTile(dest_i, dest_j, dir, canMerge);
		if (returnVal == 1)
			return 1;
		return 2;
	} else if ((numGrid[dest_i][dest_j] == numGrid[src_i][src_j]) && canMerge) {
		numGrid[dest_i][dest_j] = numGrid[src_i][src_j] << 1;
		numGrid[src_i][src_j] = 0;
		return 1;
	}
	return 0;
}

void shiftUp() {
	s8 movedATile = 0;
	s8 canMerge = 1;
	for (s8 j=0; j<4; j++) {
		canMerge = 1;
		for (s8 i=1; i<4; i++) {
			if (numGrid[i][j] > 0) {
				s8 movedThisTile = moveTile(i, j, 0, canMerge);
				switch (movedThisTile) {
					case 0: // did not move
						canMerge = 1;
						break;
					case 1: // moved but merged
						movedATile = 1;
						canMerge = 0;
						break;
					case 2: // moved and did not merge with another tile
						movedATile = 1;
						canMerge = 1;
						break;
				}
			}
		}
	}
	if (movedATile)
		generateNewTile();
	printGrid();
}

void shiftDown() {
	s8 movedATile = 0;
	s8 canMerge = 1;
	for (s8 j=0; j<4; j++) {
		canMerge = 1;
		for (s8 i=2; i>-1; i--) {
			if (numGrid[i][j] > 0) {
				s8 movedThisTile = moveTile(i, j, 1, canMerge);
				switch (movedThisTile) {
					case 0: // did not move
						canMerge = 1;
						break;
					case 1: // moved but merged
						movedATile = 1;
						canMerge = 0;
						break;
					case 2: // moved and did not merge with another tile
						movedATile = 1;
						canMerge = 1;
						break;
				}
			}
		}
	}
	if (movedATile)
		generateNewTile();
	printGrid();
}

void shiftLeft() {
	s8 movedATile = 0;
	s8 canMerge = 1;
	for (s8 i=0; i<4; i++) {
		canMerge = 1;
		for (s8 j=1; j<4; j++) {
			if (numGrid[i][j] > 0) {
				s8 movedThisTile = moveTile(i, j, 2, canMerge);
				switch (movedThisTile) {
					case 0: // did not move
						canMerge = 1;
						break;
					case 1: // moved but merged
						movedATile = 1;
						canMerge = 0;
						break;
					case 2: // moved and did not merge with another tile
						movedATile = 1;
						canMerge = 1;
						break;
				}
			}
		}
	}
	if (movedATile)
		generateNewTile();
	printGrid();
}

void shiftRight() {
	s8 movedATile = 0;
	s8 canMerge = 1;
	for (s8 i=0; i<4; i++) {
		canMerge = 1;
		for (s8 j=2; j>-1; j--) {
			if (numGrid[i][j] > 0) {
				s8 movedThisTile = moveTile(i, j, 3, canMerge);
				switch (movedThisTile) {
					case 0: // did not move
						canMerge = 1;
						break;
					case 1: // moved but merged
						movedATile = 1;
						canMerge = 0;
						break;
					case 2: // moved and did not merge with another tile
						movedATile = 1;
						canMerge = 1;
						break;
				}
			}
		}
	}
	if (movedATile)
		generateNewTile();
	printGrid();
}

void copyNumGrid() {
	for (s8 i=0; i<4; i++) {
		for (s8 j=0; j<4; j++) {
			prevNumGrid[i][j] = numGrid[i][j];
		}
	}
}

void restorePrevNumGrid() {
	for (s8 i=0; i<4; i++) {
		for (s8 j=0; j<4; j++) {
			numGrid[i][j] = prevNumGrid[i][j];
		}
	}
	printGrid();
}

void changeMusic() {
	switch (songIndex) {
		case 0:
			MODPlay_SetMOD(&play, ancient_days__mod);
			MODPlay_Start(&play);
			break;
		case 1:
			MODPlay_SetMOD(&play, morphed_dreams_s_mod);
			MODPlay_Start(&play);
			break;
		case 2:
			MODPlay_SetMOD(&play, upfront_mod);
			MODPlay_Start(&play);
			break;
		case 3:
			MODPlay_SetMOD(&play, diginnov_mod);
			MODPlay_Start(&play);
			break;
		case 4:
			MODPlay_SetMOD(&play, addicti_mod);
			MODPlay_Start(&play);
			break;
		default:
			MODPlay_SetMOD(&play, madbit___melody_mod);
			MODPlay_Start(&play);
			break;
	}
	songIndex = (songIndex+1)%5;
}

void initialize() {
	// Initialise the video system
	VIDEO_Init();

	// These functions initialise the attached controllers
	WPAD_Init();
	PAD_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	AESND_Init();
	MODPlay_Init(&play);

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 4
	// printf("\x1b[%d;%dH", 2, 4);
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	initialize();

	MODPlay_SetMOD(&play, madbit___melody_mod);
	MODPlay_Start(&play);

	initScreen();

	while(1) {

		// Call WPAD_ScanPads (Wii) and PAD_ScanPads (Gamecube) each loop, this reads the latest controller states
		WPAD_ScanPads();
		PAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressedWii = WPAD_ButtonsDown(0);
		u32 pressedGC = PAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ((pressedWii & WPAD_BUTTON_HOME) || pressedGC & PAD_BUTTON_START) {
			MODPlay_Stop(&play);
			exit(0);
		}

		if ((pressedWii & (WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_MINUS)) || pressedGC & PAD_TRIGGER_Z)
			changeMusic();

		switch (gameState) {
			case 0:
				initNumGrid();
				gameState = 1;
				break;
			case 1:
				if ((pressedWii & (WPAD_BUTTON_RIGHT | WPAD_CLASSIC_BUTTON_UP)) || pressedGC & PAD_BUTTON_UP) {
					copyNumGrid();
					shiftUp();
					if (checkForWin())
						break;
					checkForLoss();
				}
				else if ((pressedWii & (WPAD_BUTTON_LEFT | WPAD_CLASSIC_BUTTON_DOWN)) || pressedGC & PAD_BUTTON_DOWN) {
					copyNumGrid();
					shiftDown();
					if (checkForWin())
						break;
					checkForLoss();
				}
				else if ((pressedWii & (WPAD_BUTTON_UP | WPAD_CLASSIC_BUTTON_LEFT)) || pressedGC & PAD_BUTTON_LEFT) {
					copyNumGrid();
					shiftLeft();
					if (checkForWin())
						break;
					checkForLoss();
				}
				else if ((pressedWii & (WPAD_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_RIGHT)) || pressedGC & PAD_BUTTON_RIGHT) {
					copyNumGrid();
					shiftRight();
					if (checkForWin())
						break;
					checkForLoss();
				}
				else if ((pressedWii & (WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_PLUS)) || pressedGC & PAD_BUTTON_Y) {
					// initScreen();
					// gameState = 0;
					initNumGrid();
				}
				else if ((pressedWii & (WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B)) || pressedGC & PAD_BUTTON_B) {
					restorePrevNumGrid();
				}
				break;
			case 2:
				if ((pressedWii & (WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_PLUS)) || pressedGC & PAD_BUTTON_Y) {
					initScreen();
					initNumGrid();
					gameState = 1;
				}
				break;
			default:
				gameState = 0;
				break;
		}

		// if (!moveRemaining()) // game over

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
