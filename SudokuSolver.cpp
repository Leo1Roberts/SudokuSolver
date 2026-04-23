#include <iostream>
#include <string>
#include <fstream>

// Assumes file contains single line of all puzzles (only digits 0 - 9)
char** load_puzzles_from_file(const std::string& file_path, int num_puzzles) {
	std::ifstream file(file_path);

	if (!file.is_open()) {
		std::cerr << "Error opening file: " << file_path << std::endl;
		return nullptr;
	}

	std::string puzzles_str;
	std::getline(file, puzzles_str);

	char** puzzles = new char* [num_puzzles];
	for (int i = 0; i < num_puzzles; i++) {
		puzzles[i] = new char[81];
	}

	for (int p = 0; p < num_puzzles; p++)
		for (int i = 0; i < 81; i++)
			puzzles[p][i] = puzzles_str[p * 81 + i] - '0';

	return puzzles;
}

void print_puzzle(char* puzzle) {
	for (int row = 0; row < 9; row++) {
		for (int col = 0; col < 9; col++)
			std::cout << (int)puzzle[row * 9 + col] << " ";

		std::cout << std::endl;
	}
}

int main() {
	char** puzzles_ve = load_puzzles_from_file("very_easy_puzzle.txt", 15);

	for (int i = 0; i < 15; i++) {
		print_puzzle(puzzles_ve[i]);
		std::cout << std::endl;
	}

	std::getchar();
}