#include <string>
#include <iostream>
#include <fstream>
#include <windows.h>

// --- UTILITIES --- //

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

void free_puzzles(char** puzzles, int num_puzzles) {
	for (int i = 0; i < num_puzzles; i++)
		delete[] puzzles[i];
	delete[] puzzles;
}

void print_puzzle(char* puzzle) {
	for (int row = 0; row < 9; row++) {
		for (int col = 0; col < 9; col++)
			std::cout << (int)puzzle[row * 9 + col] << " ";

		std::cout << std::endl;
	}
}


// --- SOLVER --- //

							 //  (prev,next) (row-col,row-num,col-num,box-num) (options)   (prev,next,left,right,count) (constraints)
#define ROW_COL			5832 // =     2     *                4                *  9*9*9   +           5                 *    9*9*0
#define ROW_NUM			6237 // =     2     *                4                *  9*9*9   +           5                 *    9*9*1
#define COL_NUM			6642 // =     2     *                4                *  9*9*9   +           5                 *    9*9*2
#define BOX_NUM			7047 // =     2     *                4                *  9*9*9   +           5                 *    9*9*3
#define DUMMY_HEADER	7452 // =     2     *                4                *  9*9*9   +           5                 *    9*9*4
#define ARRAY_SIZE		7456 // 2 unused followed by left & right("dummy header") - reference point to start iterating over non - deleted headers

#define PREV  0
#define NEXT  1
#define LEFT  2
#define RIGHT 3
#define COUNT 4

#define   NODE_INTERVAL 2
#define    NUM_INTERVAL (4 * NODE_INTERVAL)
#define    COL_INTERVAL (9 *  NUM_INTERVAL)
#define    ROW_INTERVAL (9 *  COL_INTERVAL)
#define HEADER_INTERVAL 5

short initial_matrix[ARRAY_SIZE] = {};
short count_indices[ROW_COL] = {};

// Initialise...

void initialise() {
	for (short row = 0; row < 9; row++) {
		for (short col = 0; col < 9; col++) {
			for (short num = 0; num < 9; num++) {
				short node = row * ROW_INTERVAL + col * COL_INTERVAL + num * NUM_INTERVAL;

				short header = ROW_COL + (row * 9 + col) * HEADER_INTERVAL;
				count_indices[node] = header + COUNT;

				initial_matrix[node + PREV] = node - NUM_INTERVAL;
				initial_matrix[node + NEXT] = node + NUM_INTERVAL;
				if (num == 0) {
					initial_matrix[node + PREV] = header;
					initial_matrix[header + NEXT] = node;
				} else if (num == 8) {
					initial_matrix[node + NEXT] = header;
					initial_matrix[header + PREV] = node;
				}

				node += NODE_INTERVAL;

				header = ROW_NUM + (row * 9 + num) * HEADER_INTERVAL;
				count_indices[node] = header + COUNT;

				initial_matrix[node + PREV] = node - COL_INTERVAL;
				initial_matrix[node + NEXT] = node + COL_INTERVAL;
				if (col == 0) {
					initial_matrix[node + PREV] = header;
					initial_matrix[header + NEXT] = node;
				} else if (col == 8) {
					initial_matrix[node + NEXT] = header;
					initial_matrix[header + PREV] = node;
				}

				node += NODE_INTERVAL;

				header = COL_NUM + (col * 9 + num) * HEADER_INTERVAL;
				count_indices[node] = header + COUNT;

				initial_matrix[node + PREV] = node - ROW_INTERVAL;
				initial_matrix[node + NEXT] = node + ROW_INTERVAL;
				if (row == 0) {
					initial_matrix[node + PREV] = header;
					initial_matrix[header + NEXT] = node;
				} else if (row == 8) {
					initial_matrix[node + NEXT] = header;
					initial_matrix[header + PREV] = node;
				}

				node += NODE_INTERVAL;


				header = BOX_NUM + ((row / 3 * 3 + col / 3) * 9 + num) * HEADER_INTERVAL;
				count_indices[node] = header + COUNT;

				short box_pos = row % 3 * 3 + col % 3;
				short box_row = row - row % 3;
				short box_col = col - col % 3;
				if (box_pos == 0) {
					initial_matrix[node + PREV] = header;
					initial_matrix[header + NEXT] = node;
				} else {
					short box_pos_prev = box_pos - 1;
					initial_matrix[node + PREV] = (box_row + box_pos_prev / 3) * ROW_INTERVAL + (box_col + box_pos_prev % 3) * COL_INTERVAL + num * NUM_INTERVAL + 6;
				}
				if (box_pos == 8) {
					initial_matrix[node + NEXT] = header;
					initial_matrix[header + PREV] = node;
				} else {
					short box_pos_next = box_pos + 1;
					initial_matrix[node + NEXT] = (box_row + box_pos_next / 3) * ROW_INTERVAL + (box_col + box_pos_next % 3) * COL_INTERVAL + num * NUM_INTERVAL + 6;
				}
			}
		}
	}

	for (short header = ROW_COL; header < DUMMY_HEADER; header += HEADER_INTERVAL) {
		initial_matrix[header + LEFT] = header - HEADER_INTERVAL;
		initial_matrix[header + RIGHT] = header + HEADER_INTERVAL;
		initial_matrix[header + COUNT] = 9;
	}
	initial_matrix[ROW_COL + LEFT] = DUMMY_HEADER;
	initial_matrix[DUMMY_HEADER + LEFT] = DUMMY_HEADER - HEADER_INTERVAL;
	initial_matrix[DUMMY_HEADER + RIGHT] = ROW_COL;
}

void sudoku_solver(char* sudoku, char* solution) {
	short matrix[ARRAY_SIZE];
	memcpy(matrix, initial_matrix, ARRAY_SIZE * sizeof(short));

	short selections_made = 0;
	for (short row = 0; row < 9; row++) {
		for (short col = 0; col < 9; col++) {
			short num = sudoku[row * 9 + col];
			if (num) {
				short selected_option = row * ROW_INTERVAL + col * COL_INTERVAL + (num - 1) * NUM_INTERVAL;

				for (short satisfied_constraint = 0; satisfied_constraint < NUM_INTERVAL; satisfied_constraint += NODE_INTERVAL) {
					short selected_node = selected_option + satisfied_constraint;
					short related_node = matrix[selected_node + PREV]; // Other node satisfying the same constraint

					while (related_node != selected_node) {
						if (related_node >= ROW_COL) { // Node is a header, remove it from the list of unsatisfied contraints
							short left_header = matrix[related_node + LEFT];
							short right_header = matrix[related_node + RIGHT];
							matrix[left_header + RIGHT] = right_header;
							matrix[right_header + LEFT] = left_header;
						} else { // Found another option which satisfies the same contraint, cover it
							for (short constraint = 0; constraint < NUM_INTERVAL; constraint += NODE_INTERVAL) {
								if (constraint != satisfied_constraint) {

									short covered_node = related_node - satisfied_constraint + constraint;
									short count = count_indices[covered_node];

									if (matrix[count] == 1) {
										memset(solution, -1, 81);
										return;
									} else {
										short prev_node = matrix[covered_node + PREV];
										short next_node = matrix[covered_node + NEXT];
										matrix[prev_node + NEXT] = next_node;
										matrix[next_node + PREV] = prev_node;
										matrix[count]--;
									}
								}
							}
						}

						related_node = matrix[related_node + PREV]; // Move onto another related node
					}
				}

				selections_made++;
			}
		}
	}

	short given_selections = selections_made;
	short selections[81];

	// MRV heuristic
	short header = matrix[DUMMY_HEADER + RIGHT];
	short min_count = 10;
	short chosen_header = -1;
	while (header != DUMMY_HEADER) {
		short count = matrix[header + COUNT];
		if (count < min_count) {
			min_count = count;
			chosen_header = header;
			if (count == 1) // Lowest possible
				break; // Stop searching
		}
		header = matrix[header + RIGHT];
	}
	selections[selections_made] = chosen_header;

	short direction = 1;

	while (true) {
		short possible_node = matrix[selections[selections_made] + PREV];

		if (possible_node >= ROW_COL) { // Node is a header, all options have been exhausted
			selections_made--; // Backtrack

			if (selections_made < given_selections) {
				memset(solution, -1, 81);
				return;
			}

			possible_node = selections[selections_made];
			direction = -1; // Undo the prior selection
		}

		short selected_option = possible_node - possible_node % NUM_INTERVAL;
		selections[selections_made] = possible_node;

		short satisfied_constraint = 3 - direction * 3;
		while (satisfied_constraint >= 0 && satisfied_constraint < NUM_INTERVAL) {
			short selected_node = selected_option + satisfied_constraint;
			short related_node = matrix[selected_node + (direction == 1 ? PREV : NEXT)]; // Other node satisfying the same constraint

			while (related_node != selected_node) { // Traverse the whole contraint (header and nodes)
				if (related_node >= ROW_COL) { // Node is a header, remove it from / restore it to the list of unsatisfied contraints
					short left_header = matrix[related_node + LEFT];
					short right_header = matrix[related_node + RIGHT];
					if (direction == 1) {
						matrix[left_header + RIGHT] = right_header;
						matrix[right_header + LEFT] = left_header;
					} else {
						matrix[left_header + RIGHT] = related_node;
						matrix[right_header + LEFT] = related_node;
					}
				} else { // Found another option which satisfies the same contraint, (un)cover it
					short constraint = 3 - direction * 3;
					while (constraint >= 0 && constraint < NUM_INTERVAL) {
						if (constraint != satisfied_constraint) {
							short covered_node = related_node - satisfied_constraint + constraint;
							short count = count_indices[covered_node];

							if (direction == 1 && matrix[count] == 1)
								direction = -1;
							else {
								matrix[count] -= direction;

								short prev_node = matrix[covered_node + PREV];
								short next_node = matrix[covered_node + NEXT];
								if (direction == 1) {
									matrix[prev_node + NEXT] = next_node;
									matrix[next_node + PREV] = prev_node;
								} else {
									matrix[prev_node + NEXT] = covered_node;
									matrix[next_node + PREV] = covered_node;
								}
							}
						}
						constraint += direction * NODE_INTERVAL;
					}
				}
				related_node = matrix[related_node + (direction == 1 ? PREV : NEXT)]; // Move onto another related node
			}
			satisfied_constraint += direction * NODE_INTERVAL;
		}

		if (direction == 1) {
			selections_made++;
			if (selections_made == 81) {
				memcpy(solution, sudoku, 81);
				for (short i = given_selections; i < 81; i++) {
					short selected_option = selections[i];
					solution[selected_option / ROW_INTERVAL * 9 + (selected_option % ROW_INTERVAL) / COL_INTERVAL] = (selected_option % COL_INTERVAL) / NUM_INTERVAL + 1;
				}
				return;
			}

			// MRV heuristic
			header = matrix[DUMMY_HEADER + RIGHT];
			min_count = 10;
			chosen_header = -1;
			while (header != DUMMY_HEADER) {
				short count = matrix[header + COUNT];
				if (count < min_count) {
					min_count = count;
					chosen_header = header;
					if (count == 1) // Lowest possible
						break; // Stop searching
				}
				header = matrix[header + RIGHT];
			}
			selections[selections_made] = chosen_header;
		} else
			direction = 1;
	}
}

// --- ENTRY POINT --- //

int main() {
	char** puzzles_ve = load_puzzles_from_file("very_easy_puzzle.txt", 15);
	char** puzzles_e = load_puzzles_from_file("easy_puzzle.txt", 15);
	char** puzzles_m = load_puzzles_from_file("medium_puzzle.txt", 15);
	char** puzzles_h = load_puzzles_from_file("hard_puzzle.txt", 15);

	initialise();

	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);

	LARGE_INTEGER sum = {0};
	const int ITS = 10000;
	char solution[81];
	for (int i = 0; i < ITS; i++) {
		for (int j = 0; j < 15; j++) {
			LARGE_INTEGER start, end;
			char* s;

			//s = puzzles_ve[j];
			//QueryPerformanceCounter(&start);
			//sudoku_solver(s, solution);
			//QueryPerformanceCounter(&end);
			//sum.QuadPart += end.QuadPart - start.QuadPart;

			//s = puzzles_e[j];
			//QueryPerformanceCounter(&start);
			//sudoku_solver(s, solution);
			//QueryPerformanceCounter(&end);
			//sum.QuadPart += end.QuadPart - start.QuadPart;

			s = puzzles_m[j];
			QueryPerformanceCounter(&start);
			sudoku_solver(s, solution);
			QueryPerformanceCounter(&end);
			sum.QuadPart += end.QuadPart - start.QuadPart;

			s = puzzles_h[j];
			QueryPerformanceCounter(&start);
			sudoku_solver(s, solution);
			QueryPerformanceCounter(&end);
			sum.QuadPart += end.QuadPart - start.QuadPart;
		}
	}

	sum.QuadPart *= (LONGLONG)1e6;
	sum.QuadPart /= Frequency.QuadPart;

	std::cout << "Mean time to solve a puzzle: " << round(sum.QuadPart / ITS / 30) << " microseconds" << std::endl;
	std::getchar();

	free_puzzles(puzzles_ve, 15);
	free_puzzles(puzzles_e, 15);
	free_puzzles(puzzles_m, 15);
	free_puzzles(puzzles_h, 15);
}