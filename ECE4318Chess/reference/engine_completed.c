#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Minimal UCI engine: first legal move.
// Supports castling and promotions; no en-passant.

typedef struct {
    int from, to;
    char promo;
} Move;

typedef struct {
    char b[64];
    int white_to_move;
    int castle_wk;
    int castle_wq;
    int castle_bk;
    int castle_bq;
} Pos;

static int sq_index(const char *s) {
    int file = s[0] - 'a';
    int rank = s[1] - '1';
    return rank * 8 + file;
}

static void index_to_sq(int idx, char out[3]) {
    out[0] = (char) ('a' + (idx % 8));
    out[1] = (char) ('1' + (idx / 8));
    out[2] = 0;
}

static void pos_from_fen(Pos *p, const char *fen) {
    memset(p->b, '.', 64);
    p->white_to_move = 1;
    p->castle_wk = 0;
    p->castle_wq = 0;
    p->castle_bk = 0;
    p->castle_bq = 0;

    char buf[256];
    strncpy(buf, fen, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *save = NULL;
    char *placement = strtok_r(buf, " ", &save);
    char *stm = strtok_r(NULL, " ", &save);
    char *castling = strtok_r(NULL, " ", &save);
    if (stm) p->white_to_move = (strcmp(stm, "w") == 0);
    if (castling) {
        p->castle_wk = (strchr(castling, 'K') != NULL);
        p->castle_wq = (strchr(castling, 'Q') != NULL);
        p->castle_bk = (strchr(castling, 'k') != NULL);
        p->castle_bq = (strchr(castling, 'q') != NULL);
    }

    int rank = 7, file = 0;
    for (size_t i = 0; placement && placement[i]; i++) {
        char c = placement[i];
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }
        if (isdigit((unsigned char) c)) {
            file += c - '0';
            continue;
        }
        int idx = rank * 8 + file;
        if (idx >= 0 && idx < 64) p->b[idx] = c;
        file++;
    }
}

static void pos_start(Pos *p) {
    pos_from_fen(p, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static int is_rook_home_square(int sq, char piece) {
    return (piece == 'R' && (sq == 0 || sq == 7)) || (piece == 'r' && (sq == 56 || sq == 63));
}

static int is_white_piece(char c) { return c >= 'A' && c <= 'Z'; }

static int is_square_attacked(const Pos *p, int sq, int by_white) {
    int r = sq / 8, f = sq % 8;

    if (by_white) {
        if (r > 0 && f > 0 && p->b[(r - 1) * 8 + (f - 1)] == 'P') return 1;
        if (r > 0 && f < 7 && p->b[(r - 1) * 8 + (f + 1)] == 'P') return 1;
    } else {
        if (r < 7 && f > 0 && p->b[(r + 1) * 8 + (f - 1)] == 'p') return 1;
        if (r < 7 && f < 7 && p->b[(r + 1) * 8 + (f + 1)] == 'p') return 1;
    }

    static const int nd[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    for (int i = 0; i < 8; i++) {
        int to = sq + nd[i];
        if (to < 0 || to >= 64) continue;
        int tr = to / 8, tf = to % 8;
        int dr = tr - r;
        if (dr < 0) dr = -dr;
        int df = tf - f;
        if (df < 0) df = -df;
        if (!((dr == 1 && df == 2) || (dr == 2 && df == 1))) continue;
        char pc = p->b[to];
        if (by_white && pc == 'N') return 1;
        if (!by_white && pc == 'n') return 1;
    }

    static const int dirs[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    for (int di = 0; di < 8; di++) {
        int df = dirs[di][0], dr = dirs[di][1];
        int cr = r + dr, cf = f + df;
        while (cr >= 0 && cr < 8 && cf >= 0 && cf < 8) {
            int idx = cr * 8 + cf;
            char pc = p->b[idx];
            if (pc != '.') {
                int pc_white = is_white_piece(pc);
                if (pc_white == by_white) {
                    char up = (char) toupper((unsigned char) pc);
                    int rook_dir = (di < 4);
                    int bishop_dir = (di >= 4);
                    if (up == 'Q') return 1;
                    if (rook_dir && up == 'R') return 1;
                    if (bishop_dir && up == 'B') return 1;
                    if (up == 'K' && (abs(cr - r) <= 1 && abs(cf - f) <= 1)) return 1;
                }
                break;
            }
            cr += dr;
            cf += df;
        }
    }

    for (int rr = r - 1; rr <= r + 1; rr++) {
        for (int ff = f - 1; ff <= f + 1; ff++) {
            if (rr < 0 || rr >= 8 || ff < 0 || ff >= 8) continue;
            if (rr == r && ff == f) continue;
            char pc = p->b[rr * 8 + ff];
            if (by_white && pc == 'K') return 1;
            if (!by_white && pc == 'k') return 1;
        }
    }

    return 0;
}

static int in_check(const Pos *p, int white_king) {
    char k = white_king ? 'K' : 'k';
    int ksq = -1;
    for (int i = 0; i < 64; i++) {
        if (p->b[i] == k) {
            ksq = i;
            break;
        }
    }
    if (ksq < 0) return 1;
    return is_square_attacked(p, ksq, !white_king);
}

static Pos make_move(const Pos *p, Move m) {
    Pos np = *p;
    char piece = np.b[m.from];
    char captured = np.b[m.to];
    np.b[m.from] = '.';
    char placed = piece;
    if (m.promo && (piece == 'P' || piece == 'p')) {
        placed = is_white_piece(piece)
                     ? (char) toupper((unsigned char) m.promo)
                     : (char) tolower((unsigned char) m.promo);
    }
    np.b[m.to] = placed;

    if (piece == 'K') {
        np.castle_wk = 0;
        np.castle_wq = 0;
        if (m.from == 4 && m.to == 6) {
            np.b[7] = '.';
            np.b[5] = 'R';
        } else if (m.from == 4 && m.to == 2) {
            np.b[0] = '.';
            np.b[3] = 'R';
        }
    } else if (piece == 'k') {
        np.castle_bk = 0;
        np.castle_bq = 0;
        if (m.from == 60 && m.to == 62) {
            np.b[63] = '.';
            np.b[61] = 'r';
        } else if (m.from == 60 && m.to == 58) {
            np.b[56] = '.';
            np.b[59] = 'r';
        }
    } else if (piece == 'R') {
        if (m.from == 0) np.castle_wq = 0;
        if (m.from == 7) np.castle_wk = 0;
    } else if (piece == 'r') {
        if (m.from == 56) np.castle_bq = 0;
        if (m.from == 63) np.castle_bk = 0;
    }

    if (is_rook_home_square(m.to, captured)) {
        if (captured == 'R') {
            if (m.to == 0) np.castle_wq = 0;
            if (m.to == 7) np.castle_wk = 0;
        } else {
            if (m.to == 56) np.castle_bq = 0;
            if (m.to == 63) np.castle_bk = 0;
        }
    }

    np.white_to_move = !p->white_to_move;
    return np;
}

static void add_move(Move *moves, int *n, int from, int to, char promo) {
    moves[*n].from = from;
    moves[*n].to = to;
    moves[*n].promo = promo;
    (*n)++;
}

// Generate pawn pushes, captures, and all four promotion piece choices.
static void gen_pawn(const Pos *p, int from, int white, Move *moves, int *n) {
    int r = from / 8;
    int f = from % 8;
    int dir = white ? 1 : -1;
    int start_rank = white ? 1 : 6;
    int promo_rank = white ? 6 : 1;
    int next_r = r + dir;
    static const char promos[4] = {'q', 'r', 'b', 'n'};

    if (next_r >= 0 && next_r < 8) {
        int one = next_r * 8 + f;
        if (p->b[one] == '.') {
            if (r == promo_rank) {
                for (int i = 0; i < 4; i++) add_move(moves, n, from, one, promos[i]);
            } else add_move(moves, n, from, one, 0);

            if (r == start_rank) {
                int two_r = r + 2 * dir;
                int two = two_r * 8 + f;
                if (two_r >= 0 && two_r < 8 && p->b[two] == '.') {
                    add_move(moves, n, from, two, 0);
                }
            }
        }

        for (int df = -1; df <= 1; df += 2) {
            int cf = f + df;
            if (cf < 0 || cf >= 8) continue;
            int to = next_r * 8 + cf;
            char target = p->b[to];
            if (target == '.' || is_white_piece(target) == white) continue;

            if (r == promo_rank) {
                for (int i = 0; i < 4; i++) add_move(moves, n, from, to, promos[i]);
            } else add_move(moves, n, from, to, 0);
        }
    }
}

// Generate all knight jumps; knights ignore blocking pieces.
static void gen_knight(const Pos *p, int from, int white, Move *moves, int *n) {
    int r = from / 8;
    int f = from % 8;
    static const int jumps[8][2] = {
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
    };

    for (int i = 0; i < 8; i++) {
        int nr = r + jumps[i][1];
        int nf = f + jumps[i][0];
        if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;

        int to = nr * 8 + nf;
        char target = p->b[to];
        if (target == '.' || is_white_piece(target) != white) {
            add_move(moves, n, from, to, 0);
        }
    }
}

// Generate sliding moves in the provided directions until blocked.
static void gen_queen(const Pos *p, int from, int white, const int dirs[][2], int dcount, Move *moves, int *n) {
    int r = from / 8;
    int f = from % 8;

    for (int i = 0; i < dcount; i++) {
        int df = dirs[i][0];
        int dr = dirs[i][1];
        int nr = r + dr;
        int nf = f + df;

        while (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
            int to = nr * 8 + nf;
            char target = p->b[to];

            if (target == '.') {
                add_move(moves, n, from, to, 0);
            } else {
                if (is_white_piece(target) != white) add_move(moves, n, from, to, 0);
                break;
            }

            nr += dr;
            nf += df;
        }
    }
}

// Bishop generation reuses the generic sliding-move helper.
static void gen_bishop(const Pos *p, int from, int white, const int dirs[][2], int dcount, Move *moves, int *n) {
    gen_queen(p, from, white, dirs, dcount, moves, n);
}

// Rook generation reuses the generic sliding-move helper.
static void gen_rook(const Pos *p, int from, int white, const int dirs[][2], int dcount, Move *moves, int *n) {
    gen_queen(p, from, white, dirs, dcount, moves, n);
}

// Generate king steps plus castling when the path is clear and unattacked.
static void gen_king(const Pos *p, int from, int white, Move *moves, int *n) {
    int r = from / 8;
    int f = from % 8;

    for (int dr = -1; dr <= 1; dr++) {
        for (int df = -1; df <= 1; df++) {
            if (dr == 0 && df == 0) continue;

            int nr = r + dr;
            int nf = f + df;
            if (nr < 0 || nr >= 8 || nf < 0 || nf >= 8) continue;

            int to = nr * 8 + nf;
            char target = p->b[to];
            if (target == '.' || is_white_piece(target) != white) {
                add_move(moves, n, from, to, 0);
            }
        }
    }

    if (white && from == 4 && p->b[4] == 'K' && !in_check(p, 1)) {
        if (p->castle_wk &&
            p->b[5] == '.' && p->b[6] == '.' && p->b[7] == 'R' &&
            !is_square_attacked(p, 5, 0) && !is_square_attacked(p, 6, 0)) {
            add_move(moves, n, from, 6, 0);
        }
        if (p->castle_wq &&
            p->b[1] == '.' && p->b[2] == '.' && p->b[3] == '.' && p->b[0] == 'R' &&
            !is_square_attacked(p, 3, 0) && !is_square_attacked(p, 2, 0)) {
            add_move(moves, n, from, 2, 0);
        }
    } else if (!white && from == 60 && p->b[60] == 'k' && !in_check(p, 0)) {
        if (p->castle_bk &&
            p->b[61] == '.' && p->b[62] == '.' && p->b[63] == 'r' &&
            !is_square_attacked(p, 61, 1) && !is_square_attacked(p, 62, 1)) {
            add_move(moves, n, from, 62, 0);
        }
        if (p->castle_bq &&
            p->b[57] == '.' && p->b[58] == '.' && p->b[59] == '.' && p->b[56] == 'r' &&
            !is_square_attacked(p, 59, 1) && !is_square_attacked(p, 58, 1)) {
            add_move(moves, n, from, 58, 0);
        }
    }
}

static int pseudo_legal_moves(const Pos *p, Move *moves) {
    int n = 0;
    int us_white = p->white_to_move;
    for (int i = 0; i < 64; i++) {
        char pc = p->b[i];
        if (pc == '.') continue;
        int white = is_white_piece(pc);
        if (white != us_white) continue;
        char up = (char) toupper((unsigned char) pc);
        if (up == 'P') gen_pawn(p, i, white, moves, &n);
        else if (up == 'N') gen_knight(p, i, white, moves, &n);
        else if (up == 'B') {
            static const int d[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
            gen_bishop(p, i, white, d, 4, moves, &n);
        } else if (up == 'R') {
            static const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            gen_rook(p, i, white, d, 4, moves, &n);
        } else if (up == 'Q') {
            static const int d[8][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            gen_queen(p, i, white, d, 8, moves, &n);
        } else if (up == 'K') gen_king(p, i, white, moves, &n);
    }
    return n;
}

static int legal_moves(const Pos *p, Move *out) {
    Move tmp[256];
    int pn = pseudo_legal_moves(p, tmp);
    int n = 0;
    for (int i = 0; i < pn; i++) {
        Pos np = make_move(p, tmp[i]);
        if (!in_check(&np, !np.white_to_move)) {
            out[n++] = tmp[i];
        }
    }
    return n;
}

static void apply_uci_move(Pos *p, const char *uci) {
    if (!uci || strlen(uci) < 4) return;
    Move m;
    m.from = sq_index(uci);
    m.to = sq_index(uci + 2);
    m.promo = (strlen(uci) >= 5) ? uci[4] : 0;
    Pos np = make_move(p, m);
    *p = np;
}

static void parse_position(Pos *p, const char *line) {
    char buf[1024];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char *toks[128];
    int nt = 0;
    char *save = NULL;
    for (char *tok = strtok_r(buf, " \t\r\n", &save); tok && nt < 128; tok = strtok_r(NULL, " \t\r\n", &save)) {
        toks[nt++] = tok;
    }

    int i = 1;
    if (i < nt && strcmp(toks[i], "startpos") == 0) {
        pos_start(p);
        i++;
    } else if (i < nt && strcmp(toks[i], "fen") == 0) {
        i++;
        char fen[512] = {0};
        for (int k = 0; k < 6 && i < nt; k++, i++) {
            if (k) strcat(fen, " ");
            strcat(fen, toks[i]);
        }
        pos_from_fen(p, fen);
    }

    if (i < nt && strcmp(toks[i], "moves") == 0) {
        i++;
        for (; i < nt; i++) apply_uci_move(p, toks[i]);
    }
}

static void print_bestmove(Move m) {
    char a[3], b[3];
    index_to_sq(m.from, a);
    index_to_sq(m.to, b);
    if (m.promo) printf("bestmove %s%s%c\n", a, b, m.promo);
    else printf("bestmove %s%s\n", a, b);
    fflush(stdout);
}

int main(void) {
    Pos pos;
    pos_start(&pos);

    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = 0;
        if (!len) continue;

        if (strcmp(line, "uci") == 0) {
            printf("id name team_c\n");
            printf("id author team_c_bryan\n");
            printf("uciok\n");
            fflush(stdout);
        } else if (strcmp(line, "isready") == 0) {
            printf("readyok\n");
            fflush(stdout);
        } else if (strcmp(line, "ucinewgame") == 0) {
            pos_start(&pos);
        } else if (strncmp(line, "position", 8) == 0) {
            parse_position(&pos, line);
        } else if (strncmp(line, "go", 2) == 0) {
            Move ms[256];
            int n = legal_moves(&pos, ms);
            if (n <= 0) {
                printf("bestmove 0000\n");
                fflush(stdout);
            } else {
                print_bestmove(ms[0]);
            }
        } else if (strcmp(line, "quit") == 0) {
            break;
        }
    }
    return 0;
}
