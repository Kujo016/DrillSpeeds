#include "../include/cnc.h"
#include "../include/bit_profile.h"
#include "../include/drill_tests.h"
#include "../include/tools.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {

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

std::filesystem::path executable_path(const char* argv0){
#ifdef _WIN32
	std::wstring buffer(32768, L'\0');
	const DWORD length = GetModuleFileNameW(
		nullptr,
		buffer.data(),
		static_cast<DWORD>(buffer.size())
	);
	if (length > 0 && length < buffer.size()){
		buffer.resize(length);
		return std::filesystem::path(buffer);
	}
#endif

	std::error_code error;
	const std::filesystem::path fallback =
		std::filesystem::absolute(argv0 == nullptr ? "" : argv0, error);
	return error ? std::filesystem::current_path() : fallback;
}

}

int main(int argc, char* argv[]){
	if (argc > 1 && std::string(argv[1]) == "--self-test"){
		return RunDrillRegressionTests();
	}

	const std::filesystem::path drill_dir =
		executable_path(argc > 0 ? argv[0] : nullptr).parent_path().parent_path();
	const std::filesystem::path data_dir = drill_dir / "data";
	const std::filesystem::path bits_dir = drill_dir / "bits";
	
	bool quit = false;
	while (!quit){
		if (argc > 1 && std::string(argv[1]) == "1"){
			BitProfile bit_profile;
			if (!SelectOrCreateBitProfile(bits_dir, bit_profile)){
				return std::cin.eof() ? 0 : 1;
			}
			
			DensityChoice density_choice;
			
			bool set = false;
			bool density_set = false;
			
			while(!set || !density_set){
				set = false;
				std::cout << "Do you need to calculate density? Y/N ";
				char density_answer = '\0';
				if (!(std::cin >> density_answer)){
					return 0;
				}
				if (density_answer == 'y' || density_answer == 'Y'){
					bool param_set = false;
					int lwhm = 0;
					
					while(!param_set){
						if (!read_number("Length (cm): ", density_choice.length)){
							if (std::cin.eof()){
								return 0;
							}
							continue;
						}
						if(density_choice.length > 0.0f){
							param_set = true;
							++lwhm;
						}
					}
					param_set = false;
					while(!param_set){
						if (!read_number("Width (cm): ", density_choice.width)){
							if (std::cin.eof()){
								return 0;
							}
							continue;
						}
						if(density_choice.width > 0.0f){
							param_set = true;
							++lwhm;
						}
					}
					param_set = false;
					while(!param_set){
						if (!read_number("Height (cm): ", density_choice.height)){
							if (std::cin.eof()){
								return 0;
							}
							continue;
						}
						if(density_choice.height > 0.0f){
							param_set = true;
							++lwhm;
						}
					}
					param_set = false;
					while(!param_set){
						if (!read_number("Mass (g): ", density_choice.mass)){
							if (std::cin.eof()){
								return 0;
							}
							continue;
						}
						if(density_choice.mass > 0.0f){
							param_set = true;
							++lwhm;
						}
					}
					if (lwhm == 4){
						density_choice.calculated = true;
						density_choice.density = calculate_density(
							density_choice.length,
							density_choice.width,
							density_choice.height,
							density_choice.mass
						);
						if (density_choice.density <= 15.0f){
							density_set = true;
						}
						set = true;
					}
				}
				else if (density_answer == 'n' || density_answer == 'N') {
					density_choice.calculated = false;
					if (!read_number("SAMPLE DENSITY: ", density_choice.density)){
						if (std::cin.eof()){
							return 0;
						}
						continue;
					}
					if(density_choice.density > 0.0f){
						if(density_choice.density <= 15.0f){
							density_set = true;
						}
						set = true;
					}
				}
				
				else{
					continue;
				}
					
			}
			if (run_drill_speeds(density_choice, bit_profile, data_dir) != 0){
				if (std::cin.eof()){
					return 1;
				}
				std::cerr << "Drill speed calculation failed." << std::endl;
			}
		}
		
		
		bool to_quit = true;
		while(to_quit){
			char quit_answer = '\0';
			std::cout << "Quit? Y/N ";
			if (!(std::cin >> quit_answer)){
				return 0;
			}
			if(quit_answer == 'Y' || quit_answer == 'y'){
				quit = true;
				to_quit = false;
			}
			else if (quit_answer == 'N' || quit_answer == 'n'){
				quit = false;
				to_quit = false;
			}
			else{
				std::cout << "You must enter Y/N" << std::endl;
				to_quit = true;
			}
		}
	}
	
	
	std::system("pause");
	
	return 0;
}
