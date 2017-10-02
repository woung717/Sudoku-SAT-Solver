#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <memory.h>
#include <ctype.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

#define INPUT_FILE "test1_input"
#define DIMACS_FILE "SUDOKU_DIMACS"
#define SOLUTION_FILE "SUDOKU_SOLUTION"

#define NR 9
#define NC 9
#define NV 9

#define NSR 3
#define NSC 3

#define NBUF 32

#define FZTN(a, n) for(a = 0; a < n; a++)  // iterates from 0 to n - 1 with a
#define FMTN(a, m, n) for(a = m; a < n; a++)  // iterates from m to n - 1 with a

#define PUTAT(bitbag, i) bitbag |= (1 << i)    // set a i-th bit on bitbag
#define ISAT(bitbag, i) ((bitbag & (1 << i)) > 0)  // check whether bitbag has bit at i-th place (from LSB)

#define PTOR(p) (((p - 1) % 81) / 9)  // row encoder (0~8)
#define PTOC(p)  (((p - 1) % 81) % 9)  // col encoder (0~8)
#define PTON(p) ((((((p - 1) % 9) + ((p - 1) / 81))) % 9) + 1)  // n encoder (1~9)
#define PTOSR(p) ((((p - 1) % 81) / 9) / 3)    // sub grid row encoder (0~2)
#define PTOSC(p) ((((p - 1) % 81) % 9) / 3)    // sub grid col encoder (0~2)

char map[NR][NC] = {{0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }};  // map as a global 2d array (zero'd)

int row_hint_bag[NR] = {0, };  // hints bag for each row
int col_hint_bag[NC] = {0, };  // hints bag for each column
int sub_hint_bag[NSR][NSC] = {{0, }, {0, }, {0, }};   // hints bag for each sub-grids

void parse_prob(void);
void print_map(void);
void encoder_test(void);
void get_hints(void);
void get_clause_bag(vector<int>[NR][NV], vector<int>[NC][NV], vector<int>[NSR][NSC][NV]);
void print_line_bag(vector<int>[NR][NC]);
void print_sub_grid_bag(vector<int>[NSR][NSC][NV]);
int count_clauses(vector<int>[NR][NV], vector<int>[NC][NV], vector<int>[NSR][NSC][NV]);
void DIMACS_generator(vector<int>[NR][NV], vector<int>[NC][NV], vector<int>[NSR][NSC][NV]);
void SAT_exec(void);
int read_solution(vector<int> *);
void set_solution_map(vector<int> *);

int main(int argc, char *argv[]) {
  vector<int> col_bag[NR][NV];
  vector<int> row_bag[NC][NV];
  vector<int> sub_grid_bag[NSR][NSC][NV];
  vector<int> solutions;

  parse_prob();

  printf("Given Sudoku Map\n");
  print_map();

  get_hints();
  get_clause_bag(col_bag, row_bag, sub_grid_bag);

  DIMACS_generator(col_bag, row_bag, sub_grid_bag);

  SAT_exec();

  if(read_solution(&solutions) < 0) {
    printf("no solution\n");

    return 0;
  }
  set_solution_map(&solutions);

  printf("Sudoku Map Solution\n");
  print_map();
  
  return 0;
}

void parse_prob(void) {
  int i, j;
  FILE *fp;

  if((fp = fopen(INPUT_FILE, "r")) != NULL) {
    FZTN(i, NR) {
      char row[NBUF];

      fgets(row, NBUF, fp);  // read a line from the file
      FZTN(j, NC) map[i][j] = row[j * 2];  // put each numbers(or *) into map array, each numbers are placed at even index ("%c ")
    }
  } else {
    printf("Input File Load Failure.\n");

    return;
  }
  
  fclose(fp);
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
    // printf("p = %d row = %d col = %d n = %d \n", i, PTOR(i), PTOC(i), PTON(i));
    printf("p = %d sub-row = %d sub-col = %d\n", i, PTOSR(i), PTOSC(i));
  }
}

void get_hints(void) {
  int i, j;

  FZTN(i, NR) {
    FZTN(j, NC) {
      if((map[i][j] >= '1' && map[i][j] <= '9')) {  // if there is a non-zero value on the map
        char c = map[i][j];   // get the value
        int h = atoi(&c) - 1;  // convert to the number

        PUTAT(row_hint_bag[i], h);  // put h into the row hints bag (h-th bit must be set)
        PUTAT(col_hint_bag[j], h);  // put h into the col hints bag (h-th bit must be set)
        PUTAT(sub_hint_bag[i / NSR][j / NSC], h);  // // put h into the sub-grid hints bag (h-th bit must be set)
      }
    }
  }
}

void get_clause_bag(vector<int> col_bag[NR][NV], vector<int> row_bag[NC][NV], vector<int> sub_grid_bag[NSR][NSC][NV]) {
  int p = 1;

  do {
    if(map[PTOR(p)][PTOC(p)] != '0') continue;  // pass if there is non-zero value at map(i, j)

    if(!ISAT(row_hint_bag[PTOR(p)], (PTON(p) - 1)))  // if there is no n in row hints bag
      col_bag[PTOR(p)][PTON(p) - 1].push_back(p);  // add propositional variable p in col_bag(i, n)

    if(!ISAT(col_hint_bag[PTOC(p)], (PTON(p) - 1)))  // if there is no n in col hints bag
      row_bag[PTOC(p)][PTON(p) - 1].push_back(p);  // add propositional variable p in row_bag(j, n)

    if(!ISAT(sub_hint_bag[PTOSR(p)][PTOSC(p)], (PTON(p) - 1)))  // if there is no n in sub-grid hints bag
      sub_grid_bag[PTOSR(p)][PTOSC(p)][PTON(p) - 1].push_back(p);  // add propositional variable p in sub_grid_bag(3r, 3s, n)

  } while(++p <= NR * NC * NV);
}

void print_line_bag(vector<int> clause_bag[NR][NC]) {
  int i, j, n;

  FZTN(i, NR) {
    FZTN(j, NC) {
      FZTN(n, clause_bag[i][j].size()) {
        printf("%d | n = %d | %d \n", i, j + 1, clause_bag[i][j][n]);
      }
    }
  }
}

void print_sub_grid_bag(vector<int> clause_bag[NSR][NSC][NV]) {
  int i, j, n, c;

  FZTN(i, NSR) {
    FZTN(j, NSC) {
      FZTN(n, NV) {
        FZTN(c, clause_bag[i][j][n].size()) {
          printf("sub-row = %d | sub-col = %d | n = %d\n", i, j, clause_bag[i][j][n][c]);
        }
      }
    }
  }
}

int count_clauses(vector<int> col_bag[NR][NV], vector<int> row_bag[NC][NV], vector<int> sub_grid_bag[NSR][NSC][NV]) {
  int i, j, n, n_clause = 0;

  FZTN(i, NR) {
    FZTN(j, NV) {
      if(col_bag[i][j].size()) n_clause++;
      if(row_bag[i][j].size()) n_clause++;
    }
  }

  FZTN(i, NSR) {
    FZTN(j, NSC) {
      FZTN(n, NV) {
        if(sub_grid_bag[i][j][n].size()) n_clause++;
      }
    }
  }

  return n_clause;
}

void DIMACS_generator(vector<int> col_bag[NR][NV], vector<int> row_bag[NC][NV], vector<int> sub_grid_bag[NSR][NSC][NV]) {
  int i, j, n, c;
  FILE *fp;

  int n_clause = count_clauses(col_bag, row_bag, sub_grid_bag);

  if((fp = fopen(DIMACS_FILE, "w")) != NULL) {
    fprintf(fp, "%c %s %d %d\n", 'p', "cnf", NR * NC * NV, n_clause);

    FZTN(i, NR) {
      FZTN(n, NV) {
        if(col_bag[i][n].size() == 0) continue;

        FZTN(j, col_bag[i][n].size()) {
          fprintf(fp, "%d ", col_bag[i][n][j]);
        } fprintf(fp, "0\n");
      }
    }

    FZTN(j, NC) {
      FZTN(n, NV) {
        if(row_bag[j][n].size() == 0) continue;

        FZTN(i, row_bag[j][n].size()) {
          fprintf(fp, "%d ", row_bag[j][n][i]);
        } fprintf(fp, "0\n");
      }
    }

    FZTN(i, NSR) {
      FZTN(j, NSC) {
        FZTN(n, NV) {
          if(sub_grid_bag[i][j][n].size() == 0) continue;

          FZTN(c, sub_grid_bag[i][j][n].size()) {
            fprintf(fp, "%d ", sub_grid_bag[i][j][n][c]);
          } fprintf(fp, "0\n");
        }
      }
    }
  } else {
    printf("File Create Failure.\n");

    return;
  }

  fclose(fp);  
}

void SAT_exec(void) {
  pid_t pid = fork();

  if(pid == 0) {
    execlp("minisat", "minisat", DIMACS_FILE, SOLUTION_FILE, NULL);
    exit(127);
  } else {
    waitpid(pid, 0, 0); /* wait for child to exit */
  }
}

int read_solution(vector<int> *solution) {
  int p;
  FILE *fp;

  if((fp = fopen(SOLUTION_FILE, "r")) != NULL) {
    char satisfiability[16];

    fgets(satisfiability, 16, fp);

    if(strstr(satisfiability, "UNSAT") != NULL) return -1;

    while(1) {
      fscanf(fp, "%d", &p);

      if(!p) break;
      if(p > 0) solution->push_back(p);
    }
  } else {
    printf("Input File Load Failure.\n");

    return -1;
  }

  return 0;
}

void set_solution_map(vector<int> *solution) {
  int i;

  FZTN(i, solution->size()) {
    int p = (*solution)[i];
    char c[2];

    sprintf(c, "%d", PTON(p));
    map[PTOR(p)][PTOC(p)] = c[0];
  }
}