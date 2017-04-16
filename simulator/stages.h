#ifndef stages_h
#define stages_h
#pragma warning(disable:4146)
#pragma warning( disable : 4996 )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "simulator.h"


void IF();

void ID();

void EX();

void DM();

void WB();

void update();

void move();

void detectWriteToZero();
void detectNumberOverflow(int , int, int );
int detectMemoryOverflow(int );

int detectDataMisaaligned(int );

void update();
	

void checkStall();


void checkForwardToID();

void checkForwardToEX();

#endif 
