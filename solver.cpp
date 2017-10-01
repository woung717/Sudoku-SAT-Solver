#include <cstdio>
#include <cstdlib>
#include <vector>

#include <memory.h>
#include <ctype.h>

using namespace std;

#define INPUT "test1_input"

#define NR 9
#define NC 9
#define NV 9

#define NSR 3
#define NSC 3

#define NBUF 32

#define FZTN(a, n) for(a = 0; a < n; a++)	// iterates from 0 to n - 1 with a
#define FMTN(a, m, n) for(a = m; a < n; a++)	// iterates from m to n - 1 with a

#define PUTAT(bitbag, i) bitbag |= (1 << i)		// set a i-th bit on bitbag
#define ISAT(bitbag, i) ((bitbag & (1 << i)) > 0)	// check whether bitbag has bit at i-th place (from LSB)

#define PTOR(p) (((p - 1) % 81) / 9)	// row encoder (0~8)
#define PTOC(p)	(((p - 1) % 81) % 9)	// col encoder (0~8)
#define PTON(p) ((((((p - 1) % 9) + ((p - 1) / 81))) % 9) + 1)	// n encoder (1~9)

char map[NR][NC] = {{0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }};		// map as a global 2d array (zero'd)

int row_hint_bag[NR] = {0, };	// hints bag for each rows
int col_hint_bag[NC] = {0, };	// hints bag for each columns
int sub_hint_bag[NSR][NSC] = {{0, }, {0, }, {0, }}; 	// hints bag for each sub-grids

void parse_prob(FILE *);
void print_map(void);
void encoder_test(void);
void get_hints(void);
void get_p_bag_by_line(vector<int>[NR][NV], vector<int>[NC][NV]);
void print_p_bag(vector<int>[9][9]);

int main(int argc, char *argv[]) {
	FILE *fp;

	vector<int> col_bag[NR][NV];
	vector<int> row_bag[NC][NV];

	if((fp = fopen(INPUT, "r")) != NULL) {
		parse_prob(fp);
	} else {
		printf("Input File Load Failure.\n");

		return 0;
	}

	get_hints();
	get_p_bag_by_line(col_bag, row_bag);

	fclose(fp);

	return 0;
}

void parse_prob(FILE *fp) {
	int i, j;

	FZTN(i, NR) {
		char row[NBUF];

		fgets(row, NBUF, fp);	// read a line from the file
		FZTN(j, NC) map[i][j] = row[j * 2];	// put each numbers(or *) into map array, each numbers are placed at even index ("%c ")
	}
}

void print_map(void) {
	int i, j;

	FZTN(i, NR) {
		FZTN(j, NC) printf("%c ", map[i][j]);
		putchar('\n');
	}
}

void encoder_test(void) {
	int i;

	FMTN(i, 1, NC * NR * NV + 1) {
		printf("p = %d row = %d col = %d n = %d \n", i, PTOR(i), PTOC(i), PTON(i));
	}
}

void get_hints(void) {
	int i, j;

	FZTN(i, NR) {
		FZTN(j, NC) {
			if((map[i][j] >= '1' && map[i][j] <= '9')) {	// if there is a non-zero value on the map
				char c = map[i][j]; 	// get the value
				int h = atoi(&c) - 1;	// convert to the number

				PUTAT(row_hint_bag[i], h);	// put h into the row hints bag (h-th bit must be set)
				PUTAT(col_hint_bag[j], h);	// put h into the col hints bag (h-th bit must be set)
				PUTAT(sub_hint_bag[i / NSR][j / NSC], h);	// // put h into the sub-grid hints bag (h-th bit must be set)
			}
		}
	}
}

void get_p_bag_by_line(vector<int> col_bag[NR][NV], vector<int> row_bag[NC][NV]) {
	int p = 1;

	do {
		if(map[PTOR(p)][PTOC(p)] != '0') continue;	// pass if there is non-zero value at map(i, j)

		if(!ISAT(row_hint_bag[PTOR(p)], (PTON(p) - 1)))	// if there is no n in row hints bag
			col_bag[PTOR(p)][PTON(p) - 1].push_back(p);	// add propositional variable p in col_bag(i, n)

		if(!ISAT(col_hint_bag[PTOC(p)], (PTON(p) - 1)))	// if there is no n in col hints bag
			row_bag[PTOC(p)][PTON(p) - 1].push_back(p);	// add propositional variable p in row_bag(j, n)

	} while(++p <= NR * NC * NV);
}

void print_p_bag(vector<int> p_bag[9][9]) {
	int i, j, n;

	FZTN(i, NR) {
		FZTN(j, NV) {
			FZTN(n, p_bag[i][j].size()) {
				printf("%d | n = %d | %d \n", i, j + 1, p_bag[i][j][n]);
			}
		}
	}
}