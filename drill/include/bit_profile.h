#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

enum class ToolType {
	TwistDrill,
	CenterCuttingEndMill,
	NonCenterCuttingEndMill
};

struct BitProfile {
	int schema_version = 1;
	std::string profile_name;
	std::string profile_category;
	ToolType tool_type = ToolType::TwistDrill;
	float diameter_mm = 0.0f;
	int cutting_edge_count = 0;
	bool center_cutting = true;
	std::map<std::string, std::string> unknown_fields;
	std::filesystem::path source_path;
};

std::string ToolTypeToString(ToolType tool_type);
std::string ToolTypeLabel(ToolType tool_type);
bool ParseToolType(const std::string& value, ToolType& tool_type);
std::string NormalizeBitProfileCategory(const std::string& value);

bool LoadBitProfile(
	const std::filesystem::path& path,
	BitProfile& profile,
	std::string& error
);

bool SaveBitProfileAtomic(
	const std::filesystem::path& path,
	const BitProfile& profile,
	bool replace_existing,
	std::string& error
);

std::vector<BitProfile> ScanBitProfiles(
	const std::filesystem::path& bits_dir,
	std::vector<std::string>& warnings
);

bool SelectOrCreateBitProfile(
	const std::filesystem::path& bits_dir,
	BitProfile& profile
);
