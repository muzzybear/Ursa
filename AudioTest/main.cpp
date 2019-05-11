// SDL2 header required for the SDL_main macro
#include <SDL2/SDL.h>
#include <glad/glad.h>

#include "URSA.h"

#include <vector>
#include <algorithm>
#include <glm/gtc/random.hpp>
#include <glm/gtc/matrix_transform.hpp>

class AudioDevice {
public:
	AudioDevice() {
		SDL_InitSubSystem(SDL_INIT_AUDIO);

		SDL_AudioSpec want, have;
		SDL_memset(&want, 0, sizeof(want));
		want.freq = 48000;
		want.format = AUDIO_F32;
		want.channels = 2;
		want.samples = 4096;
		want.callback = audio_callback_dispatch;
		want.userdata = this;

		m_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
		if (m_dev == 0) {
			// TODO handle errors, failed to open audio
			assert(0);
		}

		SDL_PauseAudioDevice(m_dev, 0);
	}

	~AudioDevice() {
		SDL_CloseAudioDevice(m_dev);
	}

	// disable copying
	AudioDevice(const AudioDevice &other) = delete;
	AudioDevice& operator=(const AudioDevice &other) = delete;

private:
	static void audio_callback_dispatch(void *userdata, uint8_t *stream, int len)
	{
		reinterpret_cast<AudioDevice*>(userdata)->audio_callback(stream, len);
	}

	void audio_callback(uint8_t *stream, int len) {
		assert(len % 4 == 0);
		float *fs = reinterpret_cast<float*>(stream);
		int flen = len / 4;
		// lalala
		for (int i = 0; i < flen; i++) {
			float v = 1.0f / 256 * (i % 256) - 0.5f;
			fs[i] = v * 0.1f;
		}
	}

	SDL_AudioDeviceID m_dev;
};

int main(int argc, char *argv[])
{
	ursa::window(800, 600);

	AudioDevice audio;

	auto fonts = ursa::font_atlas();
	fonts.add_truetype(R"(c:\windows\fonts\arialbd.ttf)", { 22.0f, 36.0f });
	fonts.bake(512, 512);

	ursa::set_framefunc([&](float deltaTime) {
		ursa::clear({ 0.20f, 0.32f, 0.35f, 1.0f });

		ursa::transform_2d();

		ursa::blend_enable();
		ursa::draw_text(fonts, 0, 2, 2, "Ursa testapp");
		ursa::blend_disable();

	});

	auto events = std::make_shared<ursa::EventHandler>();
	ursa::set_eventhandler(events);

	events->hook(SDL_KEYDOWN, [&](void *e) {
		auto *event = static_cast<SDL_KeyboardEvent*>(e);
		if (event->keysym.scancode == SDL_SCANCODE_ESCAPE) {
			ursa::terminate();
		}
	});

	ursa::run();

	return 0;
}
