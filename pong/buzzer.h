/*
 *Carlos Tafoya
 *Project 3
 *November 29, 2016
 *Description: Added short spTimer to keep track of time, for use with speaker.
*/
#ifndef buzzer_included
#define buzzer_included

static short spTimer;

void buzzer_init();
void buzzer_set_period(short cycles);

#endif // included
