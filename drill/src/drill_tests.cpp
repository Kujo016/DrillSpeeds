#include "../include/drill_tests.h"

#include "../include/bit_profile.h"
#include "../include/drill_calculator.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool nearly_equal(float left, float right, float tolerance = 0.0001f){
	return std::fabs(left - right) <= tolerance;
}

void expect(bool condition, const std::string& name, int& failures){
	if (condition){
		std::cout << "[PASS] " << name << std::endl;
	}
	else{
		std::cerr << "[FAIL] " << name << std::endl;
		++failures;
	}
}

DrillCalibration wood_calibration(){
	DrillCalibration calibration;
	calibration.baseline_diameter_mm = 1.5f;
	calibration.baseline_rpm = 20000.0f;
	calibration.average_density = 0.79f;
	calibration.baseline_horizontal_feed_in_per_min = 15.0f;
	calibration.baseline_cutting_edge_count = 2;
	calibration.rpm_diameter_exponent = 0.35f;
	calibration.rpm_density_exponent = 0.15f;
	calibration.chip_load_diameter_exponent = 0.85f;
	calibration.chip_load_density_exponent = 0.7f;
	calibration.plunge_ratios = {0.19f, 0.20f, 0.21f, 0.22f, 0.23f, 0.24f, 0.25f};
	return calibration;
}

BitProfile profile_for(
	const std::string& name,
	float diameter_mm,
	ToolType tool_type = ToolType::CenterCuttingEndMill,
	bool center_cutting = true
){
	BitProfile profile;
	profile.profile_name = name;
	profile.profile_category = "test_bits";
	profile.tool_type = tool_type;
	profile.diameter_mm = diameter_mm;
	profile.cutting_edge_count = 2;
	profile.center_cutting = center_cutting;
	return profile;
}

}

int RunDrillRegressionTests(){
	int failures = 0;
	const DrillCalibration calibration = wood_calibration();
	std::string error;

	DrillCalculationResult two_mm;
	DrillCalculationResult one_mm;
	expect(
		CalculateDrillSpeeds(
			profile_for("2mm", 2.0f),
			calibration,
			0.39f,
			5.0f,
			two_mm,
			error
		),
		"calculate 2 mm wood example",
		failures
	);
	error.clear();
	expect(
		CalculateDrillSpeeds(
			profile_for("1mm", 1.0f),
			calibration,
			0.39f,
			5.0f,
			one_mm,
			error
		),
		"calculate 1 mm wood example",
		failures
	);
	expect(
		one_mm.chip_load_mm_per_tooth < two_mm.chip_load_mm_per_tooth,
		"1 mm bit receives lower chip load than 2 mm bit",
		failures
	);
	expect(
		one_mm.horizontal_feed_mm_per_min < two_mm.horizontal_feed_mm_per_min,
		"1 mm bit receives lower horizontal feed than 2 mm bit",
		failures
	);
	BitProfile four_edge_profile = profile_for("2mm-4f", 2.0f);
	four_edge_profile.cutting_edge_count = 4;
	DrillCalculationResult four_edge_result;
	error.clear();
	CalculateDrillSpeeds(
		four_edge_profile,
		calibration,
		0.39f,
		5.0f,
		four_edge_result,
		error
	);
	expect(
		nearly_equal(
			four_edge_result.chip_load_mm_per_tooth,
			two_mm.chip_load_mm_per_tooth
		) &&
		nearly_equal(
			four_edge_result.horizontal_feed_mm_per_min,
			two_mm.horizontal_feed_mm_per_min * 2.0f,
			0.01f
		),
		"cutting-edge count changes feed without changing chip load",
		failures
	);
	expect(
		nearly_equal(two_mm.plunge_ratio, 0.21666667f),
		"plunge level 5 interpolates the material table",
		failures
	);
	expect(
		!nearly_equal(two_mm.plunge_ratio, 0.39f * 0.5f),
		"vertical feed no longer uses density times plunge-level multiplier",
		failures
	);

	const float original_chip_load = two_mm.chip_load_mm_per_tooth;
	two_mm.steps_per_drill_bit_diameter = 999.0f;
	DrillCalculationResult recalculated;
	error.clear();
	CalculateDrillSpeeds(
		profile_for("2mm", 2.0f),
		calibration,
		0.39f,
		5.0f,
		recalculated,
		error
	);
	expect(
		nearly_equal(original_chip_load, recalculated.chip_load_mm_per_tooth),
		"steps value is not an input to chip-load calculation",
		failures
	);

	DrillCalculationResult non_center_result;
	error.clear();
	expect(
		CalculateDrillSpeeds(
			profile_for(
				"non-center",
				2.0f,
				ToolType::NonCenterCuttingEndMill,
				false
			),
			calibration,
			0.39f,
			5.0f,
			non_center_result,
			error
		) &&
		!non_center_result.has_direct_vertical_feed &&
		!non_center_result.entry_warning.empty(),
		"non-center-cutting end mill requires ramp or helical entry",
		failures
	);

	const auto unique_id =
		std::chrono::high_resolution_clock::now().time_since_epoch().count();
	const std::filesystem::path test_root =
		std::filesystem::temp_directory_path() /
		("vectoriso_drill_tests_" + std::to_string(unique_id));
	const std::filesystem::path bits_dir = test_root / "bits";

	BitProfile saved_profile = profile_for("TC-2MM-2F", 2.0f);
	saved_profile.profile_category = "titanium_carbide";
	saved_profile.unknown_fields["future_field"] = "preserve_me";
	const std::filesystem::path saved_path =
		bits_dir / "titanium_carbide" / "2.csv";
	std::string storage_error;
	expect(
		SaveBitProfileAtomic(saved_path, saved_profile, false, storage_error),
		"save complete 2 mm profile",
		failures
	);

	BitProfile loaded_profile;
	storage_error.clear();
	expect(
		LoadBitProfile(saved_path, loaded_profile, storage_error) &&
		loaded_profile.profile_name == saved_profile.profile_name &&
		loaded_profile.diameter_mm == saved_profile.diameter_mm &&
		loaded_profile.cutting_edge_count == saved_profile.cutting_edge_count &&
		loaded_profile.unknown_fields["future_field"] == "preserve_me",
		"load profile and preserve unknown fields",
		failures
	);

	storage_error.clear();
	expect(
		!SaveBitProfileAtomic(saved_path, saved_profile, false, storage_error),
		"existing profile is not silently overwritten",
		failures
	);

	BitProfile second_profile = profile_for("NB-1MM-2F", 1.0f);
	second_profile.profile_category = "nano_blue_coat";
	const std::filesystem::path second_path =
		bits_dir / "nano_blue_coat" / "1.csv";
	storage_error.clear();
	expect(
		SaveBitProfileAtomic(second_path, second_profile, false, storage_error),
		"save profile in second category",
		failures
	);

	const std::filesystem::path malformed_path =
		bits_dir / "bad" / "broken.csv";
	std::filesystem::create_directories(malformed_path.parent_path());
	{
		std::ofstream malformed(malformed_path);
		malformed << "field,value\n";
		malformed << "schema_version,1\n";
		malformed << "profile_name,broken\n";
	}

	std::vector<std::string> warnings;
	const std::vector<BitProfile> profiles = ScanBitProfiles(bits_dir, warnings);
	expect(
		profiles.size() == 2 && !warnings.empty(),
		"scan multiple categories and skip malformed profiles",
		failures
	);
	expect(
		profiles.size() == 2 &&
		profiles[0].profile_category == "nano_blue_coat" &&
		profiles[1].profile_category == "titanium_carbide",
		"profile listing uses deterministic sorting",
		failures
	);
	const std::string traversal_slug =
		NormalizeBitProfileCategory("../Nano Blue-Coat");
	expect(
		traversal_slug == "nano_blue_coat" &&
		traversal_slug.find("..") == std::string::npos &&
		traversal_slug.find('/') == std::string::npos &&
		traversal_slug.find('\\') == std::string::npos,
		"profile category normalization prevents path traversal",
		failures
	);

	const std::filesystem::path reordered_path =
		bits_dir / "reordered" / "3.csv";
	std::filesystem::create_directories(reordered_path.parent_path());
	{
		std::ofstream reordered(reordered_path);
		reordered << "field,value\n";
		reordered << "diameter_mm,3\n";
		reordered << "center_cutting,true\n";
		reordered << "profile_category,reordered\n";
		reordered << "cutting_edge_count,2\n";
		reordered << "tool_type,twist_drill\n";
		reordered << "units,mm\n";
		reordered << "profile_name,reordered-profile\n";
		reordered << "schema_version,1\n";
	}
	BitProfile reordered_profile;
	storage_error.clear();
	expect(
		LoadBitProfile(reordered_path, reordered_profile, storage_error) &&
		reordered_profile.diameter_mm == 3.0f &&
		reordered_profile.profile_name == "reordered-profile",
		"profile loading is independent of CSV row order",
		failures
	);

	std::error_code cleanup_error;
	std::filesystem::remove_all(test_root, cleanup_error);

	if (failures == 0){
		std::cout << "All drill regression tests passed." << std::endl;
		return 0;
	}
	std::cerr << failures << " drill regression test(s) failed." << std::endl;
	return 1;
}
