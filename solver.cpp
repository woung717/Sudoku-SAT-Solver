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
#define DIMACS_FILE "formula.txt"
#define SOLUTION_FILE "SAT_solution.txt"

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

#define RCNTOP(i, j, n) (((j + 1 <= n) ? ((n - j - 1) * 81) : ((8 - j + n) * 81)) + ((i * 9) + (j + 1)))

class ClauseBag {
  public:
    //vector<int> cell_bag[NR][NC];
    vector<int> cell_unique_bag[NR][NC][NV - 1][NV];
    vector<int> col_bag[NR][NV];
    vector<int> row_bag[NC][NV];
    vector<int> sub_grid_bag[NSR][NSC][NV];
    vector<int> unique_bag[NR][NC];
    
    int count_clause() {
      int i, j, k, n, m, n_clause = 0;

      FZTN(i, NR) {
        FZTN(j, NV) {
          if(this->col_bag[i][j].size()) n_clause++;
          if(this->row_bag[i][j].size()) n_clause++;
          // if(this->cell_bag[i][j].size()) n_clause++;

          FMTN(n, 1, NV) {
            FMTN(m, n + 1, NV + 1) {
              if(this->cell_unique_bag[i][j][n - 1][m - 1].size()) n_clause++;
            }
          }
        }
      }

      FZTN(i, NSR) {
        FZTN(j, NSC) {
           FZTN(k, NV) {
             if(this->sub_grid_bag[i][j][k].size()) n_clause++;
           }
        }
      }

      return n_clause;
    }
};

char map[NR][NC] = {{0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }, {0, }};  // map as a global 2d array (zero'd)

int row_hint_bag[NR] = {0, };  // hints bag for each row
int col_hint_bag[NC] = {0, };  // hints bag for each column
int sub_hint_bag[NSR][NSC] = {{0, }, {0, }, {0, }};   // hints bag for each sub-grids

void parse_prob(void);
void print_map(void);
void encoder_test(void);
void decoder_test(void);
void get_hints(void);
void get_clause_bag(ClauseBag *);
void print_line_bag(vector<int>[NR][NC]);
void print_sub_grid_bag(vector<int>[NSR][NSC][NV]);
void DIMACS_generator(ClauseBag *);
void SAT_exec(void);
int read_solution(vector<int> *);
void set_solution_map(vector<int> *);
int sudoku_checker(void);

int main(int argc, char *argv[]) {
  ClauseBag *cb = new ClauseBag;
  vector<int> solutions;
 
  parse_prob();

  printf("Given Sudoku Map\n");
  print_map();

  get_hints();
  get_clause_bag(cb);

  DIMACS_generator(cb);

  SAT_exec();

  if(read_solution(&solutions) < 0) {
    printf("no solution\n");

    return 0;
  }
  set_solution_map(&solutions);

  printf("Sudoku Map Solution\n");
  print_map();

  if(sudoku_checker()) {
    printf("Sudoku clear!\n");
  } else {
    printf("Wrong Solution!\n");
  }

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
    // printf("p = %d row = %d col = %d n = %d\n", i, PTOR(i), PTOC(i), PTON(i));
    printf("p = %d sub-row = %d sub-col = %d\n", i, PTOSR(i), PTOSC(i));
  }
}

void decoder_test(void) {
  int i, j, n;

  FZTN(n, NV) {
    FZTN(i, NR) {
      FZTN(j, NC) {
        int p = RCNTOP(i, j, n + 1);

        printf("n = %d i = %d j = %d p = %d\t", n + 1, i, j, p);
        if(PTON(p) == (n + 1) && PTOR(p) == i && PTOC(p) == j) printf("test pass!\n");
      }
    }
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

void get_clause_bag(ClauseBag* cb) {
  int p = 1, i, j, n, m;

  do {
    if(map[PTOR(p)][PTOC(p)] != '0') continue;  // pass if there is non-zero value at map(i, j)

    //cb->cell_bag[PTOR(p)][PTOC(p)].push_back(p);

    if(!ISAT(row_hint_bag[PTOR(p)], (PTON(p) - 1)))  // if there is no n in row hints bag
      cb->col_bag[PTOR(p)][PTON(p) - 1].push_back(p);  // add propositional variable p in cb->col_bag(i, n)

    if(!ISAT(col_hint_bag[PTOC(p)], (PTON(p) - 1)))  // if there is no n in col hints bag
      cb->row_bag[PTOC(p)][PTON(p) - 1].push_back(p);  // add propositional variable p in cb->row_bag(j, n)

    if(!ISAT(sub_hint_bag[PTOSR(p)][PTOSC(p)], (PTON(p) - 1)))  // if there is no n in sub-grid hints bag
      cb->sub_grid_bag[PTOSR(p)][PTOSC(p)][PTON(p) - 1].push_back(p);  // add propositional variable p in cb->sub_grid_bag(3r, 3s, n)

  } while(++p <= NR * NC * NV);

  FZTN(i, NR) {
    FZTN(j, NC) {
      if(map[i][j] != '0') continue;

      FMTN(n, 1, NV) {
        FMTN(m, n + 1, NV + 1) {
          cb->cell_unique_bag[i][j][n - 1][m - 1].push_back(-RCNTOP(i, j, n));
          cb->cell_unique_bag[i][j][n - 1][m - 1].push_back(-RCNTOP(i, j, m));
        }
      }
    }
  }
}

void print_line_bag(vector<int> line_bag[NR][NC]) {
  int i, j, n;

  FZTN(i, NR) {
    FZTN(j, NC) {
      FZTN(n, line_bag[i][j].size()) {
        printf("%d | n = %d | %d \n", i, j + 1, line_bag[i][j][n]);
      }
    }
  }
}

void print_sub_grid_bag(vector<int> sub_grid_bag[NSR][NSC][NV]) {
  int i, j, n, c;

  FZTN(i, NSR) {
    FZTN(j, NSC) {
      FZTN(n, NV) {
        FZTN(c, sub_grid_bag[i][j][n].size()) {
          printf("sub-row = %d | sub-col = %d | n = %d\n", i, j, sub_grid_bag[i][j][n][c]);
        }
      }
    }
  }
}

void DIMACS_generator(ClauseBag* cb) {
  int i, j, k, n, m, c;
  FILE *fp;

  if((fp = fopen(DIMACS_FILE, "w")) != NULL) {
    fprintf(fp, "%c %s %d %d\n", 'p', "cnf", NR * NC * NV, cb->count_clause());
    /*
    FZTN(i, NR) {
      FZTN(j, NC) {
        if(cb->cell_bag[i][j].size() == 0) continue;

        FZTN(n, cb->cell_bag[i][j].size()) {
          fprintf(fp, "%d ", cb->cell_bag[i][j][n]);
        } fprintf(fp, "0\n");
      }
    }
    */
    FZTN(i, NR) {
      FZTN(n, NV) {
        if(cb->col_bag[i][n].size() == 0) continue;

        FZTN(j, cb->col_bag[i][n].size()) {
          fprintf(fp, "%d ", cb->col_bag[i][n][j]);
        } fprintf(fp, "0\n");
      }
    }

    FZTN(j, NC) {
      FZTN(n, NV) {
        if(cb->row_bag[j][n].size() == 0) continue;

        FZTN(i, cb->row_bag[j][n].size()) {
          fprintf(fp, "%d ", cb->row_bag[j][n][i]);
        } fprintf(fp, "0\n");
      }
    }

    FZTN(i, NSR) {
      FZTN(j, NSC) {
        FZTN(n, NV) {
          if(cb->sub_grid_bag[i][j][n].size() == 0) continue;

          FZTN(c, cb->sub_grid_bag[i][j][n].size()) {
            fprintf(fp, "%d ", cb->sub_grid_bag[i][j][n][c]);
          } fprintf(fp, "0\n");
        }
      }
    }

    FZTN(i, 9) {
      FZTN(j, 9) {
        FMTN(n, 1, NV) {
          FMTN(m, n + 1, NV + 1) {
            if(cb->cell_unique_bag[i][j][n - 1][m - 1].size() == 0) continue;

            FZTN(k, cb->cell_unique_bag[i][j][n - 1][m - 1].size()) {
              fprintf(fp, "%d ", cb->cell_unique_bag[i][j][n - 1][m - 1][k]);
            } fprintf(fp, "0\n");
          }
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

int sudoku_checker(void) {
  int i, j, r, s;

  FZTN(i, NR) {
    int r_sum = 0, c_sum = 0;
    
    FZTN(j, NC) {
      r_sum += map[i][j];
      c_sum += map[j][i]; 
    }
    if(r_sum != 477 || c_sum != 477) return 0;
  }

  FZTN(i, NSR) {
    FZTN(j, NSC) {
      int sub_sum = 0;

      FZTN(r, NSR) {
        FZTN(s, NSC) {
          sub_sum += map[3 * i + r][3 * j + s];
        }
      }
      if(sub_sum != 477) return 0;
    }    
  }
  
  return 1;
}