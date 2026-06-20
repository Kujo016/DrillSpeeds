#pragma once

#include "bit_profile.h"

#include <string>
#include <vector>

struct DrillCalibration {
	float baseline_diameter_mm = 0.0f;
	float baseline_rpm = 0.0f;
	float average_density = 0.0f;
	float baseline_horizontal_feed_in_per_min = 0.0f;
	int baseline_cutting_edge_count = 0;
	float rpm_diameter_exponent = 0.0f;
	float rpm_density_exponent = 0.0f;
	float chip_load_diameter_exponent = 0.0f;
	float chip_load_density_exponent = 0.0f;
	std::vector<float> plunge_ratios;
};

struct DrillCalculationResult {
	float rpm = 0.0f;
	float chip_load_mm_per_tooth = 0.0f;
	float horizontal_feed_mm_per_min = 0.0f;
	float vertical_feed_mm_per_min = 0.0f;
	float plunge_ratio = 0.0f;
	float steps_per_drill_bit_diameter = 0.0f;
	bool has_direct_vertical_feed = true;
	std::string entry_warning;
};

bool CalculateDrillSpeeds(
	const BitProfile& profile,
	const DrillCalibration& calibration,
	float workpiece_density,
	float plunge_level,
	DrillCalculationResult& result,
	std::string& error
);

