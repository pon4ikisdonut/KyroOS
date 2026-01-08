#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stddef.h>

void audio_init();
void audio_play(const int16_t* buffer, size_t size);

#endif // AUDIO_H
