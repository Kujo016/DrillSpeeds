#include "../include/bit_profile.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {

std::string trim(const std::string& value){
	const std::size_t first = value.find_first_not_of(" \t\r\n");
	if (first == std::string::npos){
		return "";
	}
	const std::size_t last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

std::string to_lower(std::string value){
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character){
		return static_cast<char>(std::tolower(character));
	});
	return value;
}

bool parse_csv_row(
	const std::string& line,
	std::string& first,
	std::string& second
){
	std::vector<std::string> fields;
	std::string current;
	bool quoted = false;

	for (std::size_t index = 0; index < line.size(); ++index){
		const char character = line[index];
		if (character == '"'){
			if (quoted && index + 1 < line.size() && line[index + 1] == '"'){
				current.push_back('"');
				++index;
			}
			else{
				quoted = !quoted;
			}
		}
		else if (character == ',' && !quoted){
			fields.push_back(current);
			current.clear();
		}
		else{
			current.push_back(character);
		}
	}
	fields.push_back(current);

	if (quoted || fields.size() != 2){
		return false;
	}
	first = trim(fields[0]);
	second = trim(fields[1]);
	return true;
}

std::string escape_csv(const std::string& value){
	if (value.find_first_of(",\"\r\n") == std::string::npos){
		return value;
	}

	std::string escaped = "\"";
	for (const char character : value){
		if (character == '"'){
			escaped += "\"\"";
		}
		else{
			escaped.push_back(character);
		}
	}
	escaped.push_back('"');
	return escaped;
}

bool parse_bool(const std::string& value, bool& result){
	const std::string normalized = to_lower(trim(value));
	if (normalized == "true" || normalized == "yes" || normalized == "1"){
		result = true;
		return true;
	}
	if (normalized == "false" || normalized == "no" || normalized == "0"){
		result = false;
		return true;
	}
	return false;
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

std::string sanitize_slug(const std::string& value){
	std::string result;
	bool previous_underscore = false;

	for (const unsigned char character : trim(value)){
		if (std::isalnum(character)){
			result.push_back(static_cast<char>(std::tolower(character)));
			previous_underscore = false;
		}
		else if (character == ' ' || character == '-' || character == '_'){
			if (!result.empty() && !previous_underscore){
				result.push_back('_');
				previous_underscore = true;
			}
		}
	}

	while (!result.empty() && result.back() == '_'){
		result.pop_back();
	}
	if (result == "." || result == ".."){
		return "";
	}
	return result;
}

bool validate_profile(const BitProfile& profile, std::string& error){
	if (profile.schema_version != 1){
		error = "Unsupported schema_version.";
		return false;
	}
	if (trim(profile.profile_name).empty()){
		error = "profile_name is required.";
		return false;
	}
	if (NormalizeBitProfileCategory(profile.profile_category).empty()){
		error = "profile_category is invalid.";
		return false;
	}
	if (!std::isfinite(profile.diameter_mm) || profile.diameter_mm <= 0.0f){
		error = "diameter_mm must be greater than zero.";
		return false;
	}
	if (profile.cutting_edge_count < 1){
		error = "cutting_edge_count must be at least one.";
		return false;
	}
	if (
		profile.tool_type == ToolType::CenterCuttingEndMill &&
		!profile.center_cutting
	){
		error = "center_cutting_end_mill must have center_cutting=true.";
		return false;
	}
	if (
		profile.tool_type == ToolType::NonCenterCuttingEndMill &&
		profile.center_cutting
	){
		error = "non_center_cutting_end_mill must have center_cutting=false.";
		return false;
	}
	return true;
}

template <typename T>
bool read_number(const char* prompt, T& value){
	while (true){
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
	}
}

bool read_yes_no(const char* prompt, bool& value){
	while (true){
		std::cout << prompt;
		std::string answer;
		if (!(std::cin >> answer)){
			return false;
		}
		answer = to_lower(answer);
		if (answer == "y" || answer == "yes"){
			value = true;
			return true;
		}
		if (answer == "n" || answer == "no"){
			value = false;
			return true;
		}
		std::cout << "You must enter Y/N." << std::endl;
	}
}

bool replace_file(
	const std::filesystem::path& temporary_path,
	const std::filesystem::path& target_path,
	bool replace_existing,
	std::string& error
){
#ifdef _WIN32
	const DWORD flags =
		MOVEFILE_WRITE_THROUGH |
		(replace_existing ? MOVEFILE_REPLACE_EXISTING : 0);
	if (MoveFileExW(temporary_path.c_str(), target_path.c_str(), flags) != 0){
		return true;
	}
	error = "Could not finalize profile file. Windows error " +
		std::to_string(GetLastError()) + ".";
	return false;
#else
	std::error_code filesystem_error;
	if (replace_existing){
		std::filesystem::remove(target_path, filesystem_error);
		if (filesystem_error){
			error = "Could not replace existing profile: " + filesystem_error.message();
			return false;
		}
	}
	std::filesystem::rename(temporary_path, target_path, filesystem_error);
	if (filesystem_error){
		error = "Could not finalize profile file: " + filesystem_error.message();
		return false;
	}
	return true;
#endif
}

bool create_profile_interactive(
	const std::filesystem::path& bits_dir,
	BitProfile& profile
){
	profile = BitProfile{};

	std::cout << "PROFILE NAME: ";
	std::getline(std::cin >> std::ws, profile.profile_name);

	std::string category_input;
	std::cout << "PROFILE CATEGORY / BIT MATERIAL OR COATING: ";
	std::getline(std::cin, category_input);
	profile.profile_category = NormalizeBitProfileCategory(category_input);
	if (profile.profile_category.empty()){
		std::cerr << "Profile category is invalid." << std::endl;
		return false;
	}

	if (!read_number("DIAMETER (mm): ", profile.diameter_mm)){
		return false;
	}

	std::cout << "TOOL TYPE\n";
	std::cout << "1 twist drill\n";
	std::cout << "2 center-cutting end mill\n";
	std::cout << "3 non-center-cutting end mill\n";
	int tool_choice = 0;
	if (!read_number("CHOICE: ", tool_choice)){
		return false;
	}
	if (tool_choice == 1){
		profile.tool_type = ToolType::TwistDrill;
	}
	else if (tool_choice == 2){
		profile.tool_type = ToolType::CenterCuttingEndMill;
	}
	else if (tool_choice == 3){
		profile.tool_type = ToolType::NonCenterCuttingEndMill;
	}
	else{
		std::cerr << "Unsupported tool type." << std::endl;
		return false;
	}

	if (!read_number("CUTTING EDGE / FLUTE COUNT: ", profile.cutting_edge_count)){
		return false;
	}
	if (!read_yes_no("CENTER CUTTING? Y/N ", profile.center_cutting)){
		return false;
	}

	std::string validation_error;
	if (!validate_profile(profile, validation_error)){
		std::cerr << "Invalid profile: " << validation_error << std::endl;
		return false;
	}

	std::cout << "\nNEW BIT PROFILE\n";
	std::cout << "Name: " << profile.profile_name << "\n";
	std::cout << "Category: " << profile.profile_category << "\n";
	std::cout << "Diameter: " << profile.diameter_mm << " mm\n";
	std::cout << "Tool Type: " << ToolTypeLabel(profile.tool_type) << "\n";
	std::cout << "Cutting Edges: " << profile.cutting_edge_count << "\n";
	std::cout << "Center Cutting: " << (profile.center_cutting ? "Yes" : "No") << "\n";

	bool save_confirmed = false;
	if (!read_yes_no("SAVE PROFILE? Y/N ", save_confirmed) || !save_confirmed){
		return false;
	}

	const std::filesystem::path category_dir =
		bits_dir / profile.profile_category;
	const std::string diameter_name = format_number(profile.diameter_mm);
	std::filesystem::path target_path = category_dir / (diameter_name + ".csv");
	bool replace_existing = false;

	if (std::filesystem::exists(target_path)){
		std::cout << "PROFILE FILE ALREADY EXISTS\n";
		std::cout << "1 cancel\n";
		std::cout << "2 save with profile name\n";
		std::cout << "3 replace existing profile\n";
		int duplicate_choice = 0;
		if (!read_number("CHOICE: ", duplicate_choice)){
			return false;
		}
		if (duplicate_choice == 1){
			return false;
		}
		if (duplicate_choice == 2){
			const std::string profile_slug = sanitize_slug(profile.profile_name);
			if (profile_slug.empty()){
				std::cerr << "Profile name cannot form a safe filename." << std::endl;
				return false;
			}
			target_path =
				category_dir / (diameter_name + "_" + profile_slug + ".csv");
			if (std::filesystem::exists(target_path)){
				std::cerr << "Distinguishing profile filename also exists." << std::endl;
				return false;
			}
		}
		else if (duplicate_choice == 3){
			bool replace_confirmed = false;
			if (
				!read_yes_no("REPLACE EXISTING PROFILE? Y/N ", replace_confirmed) ||
				!replace_confirmed
			){
				return false;
			}
			replace_existing = true;
		}
		else{
			std::cerr << "Unsupported duplicate choice." << std::endl;
			return false;
		}
	}

	std::string save_error;
	if (!SaveBitProfileAtomic(target_path, profile, replace_existing, save_error)){
		std::cerr << "Could not save bit profile: " << save_error << std::endl;
		return false;
	}
	profile.source_path = target_path;
	std::cout << "PROFILE: " << target_path.string() << std::endl;
	return true;
}

}

std::string ToolTypeToString(ToolType tool_type){
	switch (tool_type){
	case ToolType::TwistDrill:
		return "twist_drill";
	case ToolType::CenterCuttingEndMill:
		return "center_cutting_end_mill";
	case ToolType::NonCenterCuttingEndMill:
		return "non_center_cutting_end_mill";
	}
	return "";
}

std::string ToolTypeLabel(ToolType tool_type){
	switch (tool_type){
	case ToolType::TwistDrill:
		return "twist drill";
	case ToolType::CenterCuttingEndMill:
		return "center-cutting end mill";
	case ToolType::NonCenterCuttingEndMill:
		return "non-center-cutting end mill";
	}
	return "unknown";
}

std::string NormalizeBitProfileCategory(const std::string& value){
	return sanitize_slug(value);
}

bool ParseToolType(const std::string& value, ToolType& tool_type){
	const std::string normalized = to_lower(trim(value));
	if (normalized == "twist_drill"){
		tool_type = ToolType::TwistDrill;
		return true;
	}
	if (normalized == "center_cutting_end_mill"){
		tool_type = ToolType::CenterCuttingEndMill;
		return true;
	}
	if (normalized == "non_center_cutting_end_mill"){
		tool_type = ToolType::NonCenterCuttingEndMill;
		return true;
	}
	return false;
}

bool LoadBitProfile(
	const std::filesystem::path& path,
	BitProfile& profile,
	std::string& error
){
	std::ifstream input(path);
	if (!input){
		error = "Could not open profile.";
		return false;
	}

	std::map<std::string, std::string> fields;
	std::string line;
	bool header_seen = false;
	while (std::getline(input, line)){
		if (trim(line).empty()){
			continue;
		}
		std::string field;
		std::string value;
		if (!parse_csv_row(line, field, value)){
			error = "Malformed two-column CSV row.";
			return false;
		}
		if (!header_seen){
			if (to_lower(field) != "field" || to_lower(value) != "value"){
				error = "Expected field,value header.";
				return false;
			}
			header_seen = true;
			continue;
		}
		if (field.empty()){
			error = "Profile contains an empty field name.";
			return false;
		}
		if (!fields.emplace(field, value).second){
			error = "Duplicate profile field: " + field;
			return false;
		}
	}
	if (!header_seen){
		error = "Profile is empty.";
		return false;
	}

	const std::set<std::string> known_fields = {
		"schema_version",
		"profile_name",
		"profile_category",
		"units",
		"tool_type",
		"diameter_mm",
		"cutting_edge_count",
		"center_cutting"
	};

	try{
		profile = BitProfile{};
		profile.schema_version = std::stoi(fields.at("schema_version"));
		profile.profile_name = fields.at("profile_name");
		profile.profile_category = fields.at("profile_category");
		if (fields.at("units") != "mm"){
			error = "Only units=mm is supported.";
			return false;
		}
		if (!ParseToolType(fields.at("tool_type"), profile.tool_type)){
			error = "Unsupported tool_type.";
			return false;
		}
		profile.diameter_mm = std::stof(fields.at("diameter_mm"));
		profile.cutting_edge_count = std::stoi(fields.at("cutting_edge_count"));
		if (!parse_bool(fields.at("center_cutting"), profile.center_cutting)){
			error = "center_cutting must be true or false.";
			return false;
		}
	}
	catch (const std::exception& exception){
		error = "Missing or invalid required profile field: ";
		error += exception.what();
		return false;
	}

	for (const auto& [field, value] : fields){
		if (known_fields.find(field) == known_fields.end()){
			profile.unknown_fields[field] = value;
		}
	}
	profile.source_path = path;

	return validate_profile(profile, error);
}

bool SaveBitProfileAtomic(
	const std::filesystem::path& path,
	const BitProfile& profile,
	bool replace_existing,
	std::string& error
){
	if (!validate_profile(profile, error)){
		return false;
	}
	if (std::filesystem::exists(path) && !replace_existing){
		error = "Profile already exists.";
		return false;
	}

	std::error_code filesystem_error;
	std::filesystem::create_directories(path.parent_path(), filesystem_error);
	if (filesystem_error){
		error = "Could not create profile directory: " + filesystem_error.message();
		return false;
	}

	const std::filesystem::path temporary_path =
		path.parent_path() / (path.filename().string() + ".tmp");
	std::ofstream output(temporary_path, std::ios::trunc);
	if (!output){
		error = "Could not create temporary profile file.";
		return false;
	}

	output << "field,value\n";
	output << "schema_version," << profile.schema_version << "\n";
	output << "profile_name," << escape_csv(profile.profile_name) << "\n";
	output << "profile_category," << escape_csv(profile.profile_category) << "\n";
	output << "units,mm\n";
	output << "tool_type," << ToolTypeToString(profile.tool_type) << "\n";
	output << "diameter_mm," << format_number(profile.diameter_mm) << "\n";
	output << "cutting_edge_count," << profile.cutting_edge_count << "\n";
	output << "center_cutting," << (profile.center_cutting ? "true" : "false") << "\n";
	for (const auto& [field, value] : profile.unknown_fields){
		output << escape_csv(field) << "," << escape_csv(value) << "\n";
	}
	output.close();
	if (!output){
		std::filesystem::remove(temporary_path, filesystem_error);
		error = "Could not finish writing temporary profile file.";
		return false;
	}

	if (!replace_file(temporary_path, path, replace_existing, error)){
		std::filesystem::remove(temporary_path, filesystem_error);
		return false;
	}
	return true;
}

std::vector<BitProfile> ScanBitProfiles(
	const std::filesystem::path& bits_dir,
	std::vector<std::string>& warnings
){
	std::vector<BitProfile> profiles;
	if (!std::filesystem::exists(bits_dir)){
		return profiles;
	}

	std::error_code iterator_error;
	std::filesystem::recursive_directory_iterator iterator(
		bits_dir,
		std::filesystem::directory_options::skip_permission_denied,
		iterator_error
	);
	const std::filesystem::recursive_directory_iterator end;
	while (iterator != end){
		if (iterator_error){
			warnings.push_back("Profile scan warning: " + iterator_error.message());
			iterator_error.clear();
			iterator.increment(iterator_error);
			continue;
		}

		const auto& entry = *iterator;
		if (
			entry.is_regular_file() &&
			to_lower(entry.path().extension().string()) == ".csv"
		){
			BitProfile profile;
			std::string error;
			if (LoadBitProfile(entry.path(), profile, error)){
				profiles.push_back(profile);
			}
			else{
				warnings.push_back(
					"Skipping malformed profile " +
					entry.path().string() + ": " + error
				);
			}
		}
		iterator.increment(iterator_error);
	}

	std::sort(profiles.begin(), profiles.end(), [](const BitProfile& left, const BitProfile& right){
		if (left.profile_category != right.profile_category){
			return left.profile_category < right.profile_category;
		}
		if (left.profile_name != right.profile_name){
			return left.profile_name < right.profile_name;
		}
		if (left.diameter_mm != right.diameter_mm){
			return left.diameter_mm < right.diameter_mm;
		}
		return left.source_path.string() < right.source_path.string();
	});
	return profiles;
}

bool SelectOrCreateBitProfile(
	const std::filesystem::path& bits_dir,
	BitProfile& profile
){
	while (true){
		std::cout << "\nDRILL BIT PROFILE\n";
		std::cout << "1 use an existing drill bit\n";
		std::cout << "2 create a new drill bit\n";
		int choice = 0;
		if (!read_number("CHOICE: ", choice)){
			return false;
		}

		if (choice == 1){
			std::vector<std::string> warnings;
			const std::vector<BitProfile> profiles =
				ScanBitProfiles(bits_dir, warnings);
			for (const std::string& warning : warnings){
				std::cerr << "[WARN] " << warning << std::endl;
			}
			if (profiles.empty()){
				std::cout << "No valid bit profiles found. Create a new bit first." << std::endl;
				continue;
			}

			for (std::size_t index = 0; index < profiles.size(); ++index){
				const BitProfile& candidate = profiles[index];
				std::cout
					<< (index + 1) << ". "
					<< candidate.profile_category << " / "
					<< candidate.profile_name << " / "
					<< candidate.diameter_mm << " mm / "
					<< ToolTypeLabel(candidate.tool_type) << " / "
					<< candidate.cutting_edge_count << " cutting edges\n";
			}

			int profile_choice = 0;
			if (!read_number("PROFILE: ", profile_choice)){
				return false;
			}
			if (
				profile_choice < 1 ||
				profile_choice > static_cast<int>(profiles.size())
			){
				std::cout << "Choose a listed profile." << std::endl;
				continue;
			}
			profile = profiles[static_cast<std::size_t>(profile_choice - 1)];
			std::cout << "PROFILE: " << profile.profile_name
				<< " | " << profile.diameter_mm << " mm"
				<< " | " << ToolTypeLabel(profile.tool_type)
				<< " | " << profile.cutting_edge_count
				<< " cutting edges" << std::endl;
			return true;
		}
		if (choice == 2){
			if (create_profile_interactive(bits_dir, profile)){
				return true;
			}
			if (std::cin.eof()){
				return false;
			}
			std::cout << "Bit profile was not created." << std::endl;
			continue;
		}
		std::cout << "Choose 1 or 2." << std::endl;
	}
}
