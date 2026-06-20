#pragma once
#include "bit_profile.h"
#include "globals.h"

struct DensityChoice{
	bool calculated = false;
	float density = 0.0f;
	float length = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	float mass = 0.0f;
};

int run_drill_speeds(
	const DensityChoice& density_choice,
	const BitProfile& bit_profile,
	const std::filesystem::path& data_dir
);
