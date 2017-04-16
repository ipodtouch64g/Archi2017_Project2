#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stages.h"

int main()
{
	initialize();
	OpenFile();

	while (!(WB_HALT || ERROR_HALT))
	{
		writeRegZero = 0;
		numOverflow = 0;
		snapShot_Reg();
		update();
		WB();
		DM();
		EX();
		ID();
		IF();
		snapShot_Buffer();
		move();
		errorDump();
	}

	return 0;
}