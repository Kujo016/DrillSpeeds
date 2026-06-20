#include "../include/cnc.h"
#include "../include/drill_calculator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

namespace {

constexpr float kMillimetersPerInch = 25.4f;

struct MaterialSelection {
	std::string name;
	int plunge_column = -1;
};

template <typename T>
bool read_number(const char* prompt, T& value){
	std::cout << prompt;
	if (std::cin >> value){
		return true;
	}

	if (std::cin.eof()){
		return false;
	}

	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::cout << "You must enter a number." << std::endl;
	return false;
}

std::string to_lower(std::string value){
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character){
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

std::string trim(const std::string& value){
	const std::size_t first = value.find_first_not_of(" \t\r\n");
	if (first == std::string::npos){
		return "";
	}
	const std::size_t last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

std::vector<std::string> split_line(const std::string& line, char delimiter){
	std::vector<std::string> fields;
	std::stringstream stream(line);
	std::string field;
	while (std::getline(stream, field, delimiter)){
		fields.push_back(trim(field));
	}
	if (!line.empty() && line.back() == delimiter){
		fields.emplace_back();
	}
	return fields;
}

std::string format_number(float value){
	std::ostringstream stream;
	stream << std::fixed << std::setprecision(6) << value;
	std::string result = stream.str();

	while (!result.empty() && result.back() == '0'){
		result.pop_back();
	}
	if (!result.empty() && result.back() == '.'){
		result.pop_back();
	}

	return result;
}

std::filesystem::path create_report_path(
	const std::filesystem::path& log_dir,
	const std::string& base_name
){
	std::filesystem::path report_path = log_dir / (base_name + ".txt");
	int report_number = 2;

	while (std::filesystem::exists(report_path)){
		report_path = log_dir / (base_name + "_" + std::to_string(report_number) + ".txt");
		++report_number;
	}

	return report_path;
}

bool select_material(MaterialSelection& material){
	while (true){
		std::cout << "MATERIAL\n";
		std::cout << "1 wood\n";
		std::cout << "2 polycarbonate\n";
		std::cout << "3 aluminum\n";
		std::cout << "4 brass\n";

		std::string answer;
		if (!(std::cin >> answer)){
			return false;
		}
		answer = to_lower(answer);
		if (answer == "1" || answer == "wood"){
			material = {"Wood", 1};
			return true;
		}
		if (answer == "2" || answer == "polycarbonate"){
			material = {"Polycarbonate", 2};
			return true;
		}
		if (answer == "3" || answer == "aluminum"){
			material = {"Aluminum", 3};
			return true;
		}
		if (answer == "4" || answer == "brass"){
			material = {"Brass", 4};
			return true;
		}
		std::cout << "Choose a material by number or name." << std::endl;
	}
}

bool load_exponents(
	const std::filesystem::path& path,
	const std::string& material_name,
	DrillCalibration& calibration,
	std::string& error
){
	std::ifstream input(path);
	if (!input){
		error = "Could not open drill exponent data: " + path.string();
		return false;
	}

	std::string line;
	std::getline(input, line);
	while (std::getline(input, line)){
		const std::vector<std::string> fields = split_line(line, ',');
		if (fields.size() < 5 || fields[0] != material_name){
			continue;
		}
		try{
			calibration.rpm_diameter_exponent = std::stof(fields[1]);
			calibration.rpm_density_exponent = std::stof(fields[2]);
			calibration.chip_load_diameter_exponent = std::stof(fields[3]);
			calibration.chip_load_density_exponent = std::stof(fields[4]);
			return true;
		}
		catch (const std::exception& exception){
			error = "Invalid exponent data for " + material_name + ": " + exception.what();
			return false;
		}
	}

	error = "No exponent row found for " + material_name + ".";
	return false;
}

bool load_baseline(
	const std::filesystem::path& path,
	const std::string& material_name,
	DrillCalibration& calibration,
	std::string& error
){
	std::ifstream input(path);
	if (!input){
		error = "Could not open drill baseline data: " + path.string();
		return false;
	}

	std::string line;
	std::getline(input, line);
	while (std::getline(input, line)){
		const std::vector<std::string> fields = split_line(line, ',');
		if (fields.size() < 7 || fields[0] != material_name){
			continue;
		}
		try{
			calibration.baseline_diameter_mm = std::stof(fields[1]);
			calibration.baseline_rpm = std::stof(fields[2]);
			calibration.average_density = std::stof(fields[3]);
			calibration.baseline_horizontal_feed_in_per_min = std::stof(fields[4]);
			// Current material baselines are calibrated with two cutting edges.
			// Add dedicated baseline rows when multi-fluted calibration is available.
			calibration.baseline_cutting_edge_count = std::stoi(fields[6]);
			return true;
		}
		catch (const std::exception& exception){
			error = "Invalid baseline data for " + material_name + ": " + exception.what();
			return false;
		}
	}

	error = "No baseline row found for " + material_name + ".";
	return false;
}

bool load_plunge_ratios(
	const std::filesystem::path& path,
	const MaterialSelection& material,
	DrillCalibration& calibration,
	std::string& error
){
	std::ifstream input(path);
	if (!input){
		error = "Could not open plunge calibration data: " + path.string();
		return false;
	}

	std::string line;
	if (!std::getline(input, line)){
		error = "Plunge calibration data is empty.";
		return false;
	}
	const std::vector<std::string> header = split_line(line, '\t');
	if (
		material.plunge_column < 1 ||
		material.plunge_column >= static_cast<int>(header.size()) ||
		header[static_cast<std::size_t>(material.plunge_column)] != material.name
	){
		error = "Plunge calibration header does not match " + material.name + ".";
		return false;
	}

	while (std::getline(input, line)){
		const std::vector<std::string> fields = split_line(line, '\t');
		if (material.plunge_column >= static_cast<int>(fields.size())){
			error = "Malformed plunge calibration row.";
			return false;
		}
		try{
			calibration.plunge_ratios.push_back(
				std::stof(fields[static_cast<std::size_t>(material.plunge_column)])
			);
		}
		catch (const std::exception& exception){
			error = "Invalid plunge calibration value: ";
			error += exception.what();
			return false;
		}
	}

	if (calibration.plunge_ratios.size() < 2){
		error = "At least two plunge calibration values are required.";
		return false;
	}
	return true;
}

bool load_calibration(
	const std::filesystem::path& data_dir,
	const MaterialSelection& material,
	DrillCalibration& calibration,
	std::string& error
){
	return
		load_baseline(
			data_dir / "drill_base.csv",
			material.name,
			calibration,
			error
		) &&
		load_exponents(
			data_dir / "exponents.csv",
			material.name,
			calibration,
			error
		) &&
		load_plunge_ratios(
			data_dir / "plunge_level.csv",
			material,
			calibration,
			error
		);
}

}

int run_drill_speeds(
	const DensityChoice& density_choice,
	const BitProfile& bit_profile,
	const std::filesystem::path& data_dir
){
	MaterialSelection material;
	if (!select_material(material)){
		return 1;
	}
	std::cout << material.name << std::endl;

	float plunge_level = 0.0f;
	while (true){
		if (!read_number("PLUNGE LEVEL (1-10): ", plunge_level)){
			return 1;
		}
		if (plunge_level >= 1.0f && plunge_level <= 10.0f){
			break;
		}
	}

	DrillCalibration calibration;
	std::string error;
	if (!load_calibration(data_dir, material, calibration, error)){
		std::cerr << error << std::endl;
		return 1;
	}

	DrillCalculationResult result;
	if (!CalculateDrillSpeeds(
		bit_profile,
		calibration,
		density_choice.density,
		plunge_level,
		result,
		error
	)){
		std::cerr << error << std::endl;
		return 1;
	}

	std::cout << "\nPROFILE: " << bit_profile.profile_name << std::endl;
	std::cout << "MATERIAL: " << material.name << std::endl;
	std::cout << "RPM: " << result.rpm << " rpm" << std::endl;
	std::cout << "CHIP LOAD: " << result.chip_load_mm_per_tooth
		<< " mm/tooth" << std::endl;
	std::cout << "HORIZONTAL FEED: " << result.horizontal_feed_mm_per_min
		<< " mm/min" << std::endl;
	std::cout << "HORIZONTAL FEED: "
		<< result.horizontal_feed_mm_per_min / kMillimetersPerInch
		<< " in/min" << std::endl;
	if (result.has_direct_vertical_feed){
		std::cout << "VERTICAL FEED: " << result.vertical_feed_mm_per_min
			<< " mm/min" << std::endl;
		std::cout << "VERTICAL FEED: "
			<< result.vertical_feed_mm_per_min / kMillimetersPerInch
			<< " in/min" << std::endl;
	}
	else{
		std::cout << "VERTICAL FEED: N/A" << std::endl;
		std::cout << "ENTRY: " << result.entry_warning << std::endl;
	}
	std::cout << "PLUNGE RATIO: " << result.plunge_ratio << std::endl;
	std::cout << "STEPS PER DRILL BIT DIA: "
		<< result.steps_per_drill_bit_diameter << std::endl;

	const std::filesystem::path log_dir = data_dir.parent_path() / "log";
	std::error_code directory_error;
	std::filesystem::create_directories(log_dir, directory_error);
	if (directory_error){
		std::cerr << "Could not create report directory: "
			<< directory_error.message() << std::endl;
		return 1;
	}

	const std::string report_base_name =
		format_number(bit_profile.diameter_mm) + "_" +
		material.name + "_" +
		format_number(density_choice.density);
	const std::filesystem::path report_path =
		create_report_path(log_dir, report_base_name);
	std::ofstream report(report_path);
	if (!report){
		std::cerr << "Could not create report: " << report_path << std::endl;
		return 1;
	}

	report << "DRILL SPEED REPORT\n\n";
	report << "BIT PROFILE\n";
	report << "Profile Name: " << bit_profile.profile_name << "\n";
	report << "Profile Category: " << bit_profile.profile_category << "\n";
	report << "Tool Type: " << ToolTypeLabel(bit_profile.tool_type) << "\n";
	report << "Diameter: " << bit_profile.diameter_mm << " mm\n";
	report << "Cutting Edge Count: " << bit_profile.cutting_edge_count << "\n";
	report << "Center Cutting: " << (bit_profile.center_cutting ? "Yes" : "No") << "\n\n";
	report << "WORKPIECE\n";
	report << "Density Source: "
		<< (density_choice.calculated ? "Calculated" : "Entered") << "\n";
	if (density_choice.calculated){
		report << "Length: " << density_choice.length << " cm\n";
		report << "Width: " << density_choice.width << " cm\n";
		report << "Height: " << density_choice.height << " cm\n";
		report << "Mass: " << density_choice.mass << " g\n";
	}
	report << "Density: " << density_choice.density << " g/cm3\n";
	report << "Material: " << material.name << "\n";
	report << "Plunge Level: " << plunge_level << " / 10\n\n";
	report << "RESULTS\n";
	report << "RPM: " << result.rpm << " rpm\n";
	report << "Chip Load: " << result.chip_load_mm_per_tooth << " mm/tooth\n";
	report << "Horizontal Feed: " << result.horizontal_feed_mm_per_min << " mm/min\n";
	report << "Horizontal Feed: "
		<< result.horizontal_feed_mm_per_min / kMillimetersPerInch
		<< " in/min\n";
	if (result.has_direct_vertical_feed){
		report << "Vertical Feed: " << result.vertical_feed_mm_per_min << " mm/min\n";
		report << "Vertical Feed: "
			<< result.vertical_feed_mm_per_min / kMillimetersPerInch
			<< " in/min\n";
	}
	else{
		report << "Vertical Feed: N/A\n";
		report << "Entry Requirement: " << result.entry_warning << "\n";
	}
	report << "Plunge Ratio: " << result.plunge_ratio << "\n";
	report << "Steps Per Drill Bit Diameter: "
		<< result.steps_per_drill_bit_diameter << "\n";
	report.close();

	std::cout << "REPORT: " << report_path.string() << std::endl;
	return 0;
}
