
#include <stdio.h>
#include <string.h>
#include "simulator.h"
int main()
{
	initialize();
	OpenFile();

	while (!(WB_HALT || ERROR_HALT))
	{
		writeRegZero = false;
		numOverflow = false;
		snapShot_Reg();

	}

	return 0;
}