#ifndef __AUDIOPLAY_H_
#define __AUDIOPLAY_H_

#include "main.h"
#include "wav.h"

extern __wavctrl wavctrl;
void Record_Play_Init(void);
uint8_t Play_music(u8* pname);

#endif
