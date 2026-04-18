#include "core/sf.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

/* Mock playback_speed to test the logic in isolation */
double mock_playback_speed = 1.0;

static sample_format_t test_get_output_sf(sample_format_t buffer_sf, double playback_speed)
{
	sample_format_t sf = buffer_sf;

	if (playback_speed != 1.0) {
		unsigned int rate = (unsigned int)(sf_get_rate(sf) * playback_speed + 0.5);
		if (rate < 1000)
			rate = 1000;
		if (rate > 192000)
			rate = 192000;
		sf = (sf & ~SF_RATE_MASK) | sf_rate(rate);
	}
	return sf;
}

int main(void)
{
	printf("Testing player sample rate logic...\n");

	sample_format_t base_sf = sf_rate(44100) | sf_channels(2) | sf_bits(16);

	/* 1. Normal speed */
	assert(sf_get_rate(test_get_output_sf(base_sf, 1.0)) == 44100);

	/* 2. 1.5x speed */
	assert(sf_get_rate(test_get_output_sf(base_sf, 1.5)) == 66150);

	/* 3. 0.5x speed */
	assert(sf_get_rate(test_get_output_sf(base_sf, 0.5)) == 22050);

	/* 4. Lower bound (0.01x -> 1000Hz) */
	assert(sf_get_rate(test_get_output_sf(base_sf, 0.01)) == 1000);

	/* 5. Upper bound (10.0x -> 192000Hz) */
	assert(sf_get_rate(test_get_output_sf(base_sf, 10.0)) == 192000);

	printf("All player logic tests passed!\n");
	return 0;
}
