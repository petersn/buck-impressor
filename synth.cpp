// Synthesize.

#include <cstdint>
#include <vector>
#include <random>
#include <iostream>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

std::random_device rd;
std::mt19937 rng(rd()); // Ugh, only 32 bits of seed.

static void alsa_test_error(int return_code) {
	if (return_code >= 0)
		return;
	std::cerr << "ALSA error: " << snd_strerror(return_code) << std::endl;
	exit(1);
}

struct MIDIContext {
	snd_seq_t* seq;

	MIDIContext() {
		alsa_test_error(snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0));
		// TODO, make work.
	}
};

struct PCMContext {
	snd_pcm_t* handle;
	unsigned int sample_rate;
	snd_pcm_uframes_t frames_per_window;

	PCMContext(unsigned int sample_rate, snd_pcm_uframes_t frames_per_window)
		: sample_rate(sample_rate), frames_per_window(frames_per_window)
	{
		alsa_test_error(snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0));
		snd_pcm_hw_params_t* params;
		snd_pcm_hw_params_alloca(&params);
		alsa_test_error(snd_pcm_hw_params_any(handle, params));
		alsa_test_error(snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED));
		alsa_test_error(snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE));
		alsa_test_error(snd_pcm_hw_params_set_channels(handle, params, 2));
		int dir = 0;
		alsa_test_error(snd_pcm_hw_params_set_rate_near(handle, params,  &this->sample_rate, &dir));
		alsa_test_error(snd_pcm_hw_params_set_period_size_near(handle, params, &this->frames_per_window, &dir));
		alsa_test_error(snd_pcm_hw_params(handle, params));
		// Do these do what I think they do?
		// Augh, I don't understand ALSA.
		alsa_test_error(snd_pcm_hw_params_get_rate(params, &this->sample_rate, &dir));
		alsa_test_error(snd_pcm_hw_params_get_period_size(params, &this->frames_per_window, &dir));
	}

	void write(uint16_t* buffer) {
		int rc = snd_pcm_writei(handle, buffer, frames_per_window);
		if (rc == -EPIPE) {
//			std::cerr << "Underrun." << std::endl;
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			std::cerr << "Error:" << snd_strerror(rc) << std::endl;
		} else if (rc != frames_per_window) {
			std::cerr << "Short write of " << rc << " frames, instead of " << frames_per_window << std::endl;
		}
	}
};

int main(int argc, const char** argv) {
	PCMContext pcm_ctx(48000, 32);
	std::cout << "Sample rate: " << pcm_ctx.sample_rate << "   Frames per window: " << pcm_ctx.frames_per_window << std::endl;
	std::vector<uint16_t> samples(pcm_ctx.frames_per_window * 2);
	while (true) {
		for (int i = 0; i < samples.size(); i++)
			samples[i] = std::uniform_int_distribution<int>(-(2<<15), 2<<15)(rng);
		pcm_ctx.write(&samples[0]);
	}
}

