#include <iostream>
#include <cstdint>
#include <vector>
#include <deque>
#include "SDL2/include/SDL.h"
#include "bus.h"
#define internal static


int SamplesPerSecond = 44100;			//	resolution
bool SoundIsPlaying = false;
float volume = 0.5;
vector<float> Mixbuf;

internal void SDLInitAudio(int32_t SamplesPerSecond, int32_t BufferSize)
{
	SDL_AudioSpec AudioSettings = { 0 };

	AudioSettings.freq = SamplesPerSecond;
	AudioSettings.format = AUDIO_F32SYS;		//	One of the modes that doesn't produce a high frequent pitched tone when having silence
	AudioSettings.channels = 2;
	AudioSettings.samples = BufferSize;

	SDL_OpenAudio(&AudioSettings, 0);

}

void stepAPU(unsigned char cycles) {

	//	send audio data to device; buffer is times 4, because we use floats now, which have 4 bytes per float, and buffer needs to have information of amount of bytes to be used
	SDL_QueueAudio(1, Mixbuf.data(), Mixbuf.size() * 4);

	//TODO: we could, instead of just idling everything until music buffer is drained, at least call stepPPU(0), to have a constant draw cycle, and maybe have a smoother drawing?
	while (SDL_GetQueuedAudioSize(1) > 4096 * 4) {
	}

}

void initAPU() {

	SDL_setenv("SDL_AUDIODRIVER", "directsound", 1);
	//SDL_setenv("SDL_AUDIODRIVER", "disk", 1);
	SDL_Init(SDL_INIT_AUDIO);

	// Open our audio device; Sample Rate will dictate the pace of our synthesizer
	SDLInitAudio(44100, 1024);

	if (!SoundIsPlaying)
	{
		SDL_PauseAudio(0);
		SoundIsPlaying = true;
	}
}

void stopSPU() {
	SDL_Quit();
	SoundIsPlaying = false;
}