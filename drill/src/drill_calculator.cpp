#include "../include/drill_calculator.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kMillimetersPerInch = 25.4f;

float interpolate_plunge_ratio(
	const std::vector<float>& ratios,
	float plunge_level
){
	if (ratios.size() == 1){
		return ratios.front();
	}

	const float normalized_level = (plunge_level - 1.0f) / 9.0f;
	const float position = normalized_level * static_cast<float>(ratios.size() - 1);
	const std::size_t lower_index = static_cast<std::size_t>(std::floor(position));
	const std::size_t upper_index = std::min(lower_index + 1, ratios.size() - 1);
	const float fraction = position - static_cast<float>(lower_index);
	return ratios[lower_index] +
		(ratios[upper_index] - ratios[lower_index]) * fraction;
}

bool finite_positive(float value){
	return std::isfinite(value) && value > 0.0f;
}

}

bool CalculateDrillSpeeds(
	const BitProfile& profile,
	const DrillCalibration& calibration,
	float workpiece_density,
	float plunge_level,
	DrillCalculationResult& result,
	std::string& error
){
	if (!finite_positive(profile.diameter_mm)){
		error = "Bit diameter must be greater than zero.";
		return false;
	}
	if (profile.cutting_edge_count < 1){
		error = "Cutting-edge count must be at least one.";
		return false;
	}
	if (
		!finite_positive(calibration.baseline_diameter_mm) ||
		!finite_positive(calibration.baseline_rpm) ||
		!finite_positive(calibration.average_density) ||
		!finite_positive(calibration.baseline_horizontal_feed_in_per_min) ||
		calibration.baseline_cutting_edge_count < 1
	){
		error = "Material baseline calibration is invalid.";
		return false;
	}
	if (!finite_positive(workpiece_density)){
		error = "Workpiece density must be greater than zero.";
		return false;
	}
	if (!std::isfinite(plunge_level) || plunge_level < 1.0f || plunge_level > 10.0f){
		error = "Plunge level must be between 1 and 10.";
		return false;
	}
	if (calibration.plunge_ratios.empty()){
		error = "Material plunge calibration is missing.";
		return false;
	}
	for (const float ratio : calibration.plunge_ratios){
		if (!finite_positive(ratio)){
			error = "Material plunge calibration contains an invalid ratio.";
			return false;
		}
	}

	result = DrillCalculationResult{};
	result.rpm =
		calibration.baseline_rpm *
		std::pow(
			calibration.baseline_diameter_mm / profile.diameter_mm,
			calibration.rpm_diameter_exponent
		) *
		std::pow(
			calibration.average_density / workpiece_density,
			calibration.rpm_density_exponent
		);

	const float baseline_chip_load_in_per_tooth =
		calibration.baseline_horizontal_feed_in_per_min /
		(calibration.baseline_rpm *
			static_cast<float>(calibration.baseline_cutting_edge_count));

	const float chip_load_in_per_tooth =
		baseline_chip_load_in_per_tooth *
		std::pow(
			profile.diameter_mm / calibration.baseline_diameter_mm,
			calibration.chip_load_diameter_exponent
		) *
		std::pow(
			calibration.average_density / workpiece_density,
			calibration.chip_load_density_exponent
		);

	result.chip_load_mm_per_tooth =
		chip_load_in_per_tooth * kMillimetersPerInch;
	result.horizontal_feed_mm_per_min =
		result.rpm *
		static_cast<float>(profile.cutting_edge_count) *
		result.chip_load_mm_per_tooth;
	result.plunge_ratio =
		interpolate_plunge_ratio(calibration.plunge_ratios, plunge_level);
	result.steps_per_drill_bit_diameter = profile.diameter_mm * 2.0f;

	if (profile.tool_type == ToolType::NonCenterCuttingEndMill || !profile.center_cutting){
		result.has_direct_vertical_feed = false;
		result.entry_warning =
			"Direct plunge is not recommended; use a ramp or helical entry.";
	}
	else{
		result.vertical_feed_mm_per_min =
			result.horizontal_feed_mm_per_min * result.plunge_ratio;
	}

	if (
		!finite_positive(result.rpm) ||
		!finite_positive(result.chip_load_mm_per_tooth) ||
		!finite_positive(result.horizontal_feed_mm_per_min) ||
		!finite_positive(result.steps_per_drill_bit_diameter)
	){
		error = "Calculated drill speeds are not finite and positive.";
		return false;
	}
	if (
		result.has_direct_vertical_feed &&
		!finite_positive(result.vertical_feed_mm_per_min)
	){
		error = "Calculated vertical feed is not finite and positive.";
		return false;
	}

	return true;
}

