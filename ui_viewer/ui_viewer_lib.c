#include "ui_viewer_lib.h"
#include <stdio.h>
#include <stdlib.h>

__attribute__((constructor)) void begin(void) {
	printf("I'm a constructor\n");
}

__attribute__((destructor)) void end (void)
{
	printf("I'm a destructor\n");
}
