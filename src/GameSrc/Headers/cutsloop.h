
#ifndef __CUTSLOOP_H
#define __CUTSLOOP_H

// Includes

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines

// CC: These are all wrong
#define START_CUTSCENE 0
#define DEATH_CUTSCENE 1
#define WIN_CUTSCENE   2

// Prototypes

void cutscene_loop(void);
void cutscene_start(void);
void cutscene_exit(void);
short play_cutscene(int id, bool something);

// Globals

#endif // __CUTSLOOP_H