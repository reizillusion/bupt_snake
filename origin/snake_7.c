/*
 * 文件: snake.c
 * 说明: 贪吃蛇交互程序。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ROWS 20
#define COLS 20
#define MAXSN 400
#define HCAP 1601

typedef struct {
    int r;
    int c;
    int f;
} HNode;

typedef struct {
    char grid[ROWS][COLS + 1];
    int srow[MAXSN];
    int scol[MAXSN];
    int slen;
    int bodyid[ROWS][COLS];
    int wallmap[ROWS][COLS];
    int deadmap[ROWS][COLS];
    int visitcnt[ROWS][COLS];
    int curdir;
    int score;
    int nval;
    int stepcnt;
    int last_food_step;
    clock_t start_clock;
    int foodr;
    int foodc;
    int planr[MAXSN];
    int planc[MAXSN];
    int planlen;
    int planpos;
    int plantargetr;
    int plantargetc;
    HNode hp[HCAP];
    int hpsz;
    int tmp_sr[MAXSN];
    int tmp_sc[MAXSN];
    int tmp_slen;
} Game;

static int dir_row_delta(int dir);
static int dir_col_delta(int dir);
static void hp_push(Game *game, int r, int c, int f);
static HNode hp_pop(Game *game);
static int mhdist(int r1, int c1, int r2, int c2);
static int grow_at(const Game *game, int eating, int move_num);
static void rebuild_bodyid(Game *game, const int sr[], const int sc[],
                           int len);
static void build_occ_map(const int sr[], const int sc[], int len,
                          int occ[ROWS][COLS]);
static void read_input(Game *game);
static void init_game(Game *game);
static int astar_state(Game *game, const int sr[], const int sc[],
                       int len, int step0, int dir0,
                       int tgtr, int tgtc,
                       int pathr[], int pathc[], int *fdir);
static int sim_follow_state(Game *game, const int in_sr[],
                            const int in_sc[], int in_len, int step0,
                            int food_tr, int food_tc,
                            int pathr[], int pathc[], int plen,
                            int out_sr[], int out_sc[],
                            int *out_len, int *out_dir);
static int state_space(const Game *game, const int sr[],
                       const int sc[], int len);
static int state_options(const Game *game, const int sr[],
                         const int sc[], int len, int dir0, int step0);
static int state_wall_touch(const Game *game, const int sr[],
                            const int sc[], int len);
static int static_open_degree(const Game *game, int r, int c);
static void build_static_dead_map(Game *game);
static int static_dead_cell(const Game *game, int r, int c);
static int elapsed_ms(const Game *game);
static int sim_follow_global(Game *game, int pathr[], int pathc[],
                             int plen);
static int sim_safe_like(const Game *game, int min_space);
static int flood_eval(const Game *game, int startr, int startc,
                      int blk[ROWS][COLS], int tailr, int tailc,
                      int *tail_ok);
static int choose_direction_low(Game *game);
static int safe_after_path(Game *game, const int in_sr[],
                           const int in_sc[], int in_len, int step0,
                           int food_tr, int food_tc,
                           int pathr[], int pathc[], int plen);
static int sim_one_step(const Game *game, int d, int out_sr[],
                        int out_sc[], int *out_len,
                        int *eating_out, int *growing_out);
static int direction_to_cell(const Game *game, int r, int c);
static int follow_cached_plan(Game *game);
static int choose_direction_fast(Game *game);
static int choose_finish_direction(const Game *game);
static int should_finish_fast(const Game *game, int elapsed);
static int choose_direction(Game *game);
static int do_move(Game *game, int d);
static void print_final(const Game *game);

int main(void)
{
    Game game;
    int a;
    int b;
    int d;

    memset(&game, 0, sizeof(game));
    read_input(&game);
    init_game(&game);

    game.score = 0;
    game.curdir = 0;
    game.stepcnt = 0;
    game.last_food_step = 0;
    game.start_clock = clock();
    game.planlen = 0;
    game.planpos = 0;
    game.plantargetr = -1;
    game.plantargetc = -1;

    for (;;) {
        d = choose_direction(&game);
        printf("%c\n%d\n", "WASD"[d], game.score);
        fflush(stdout);

        scanf("%d %d", &a, &b);
        if (a == 100 && b == 100) {
            print_final(&game);
            return 0;
        }

        if (do_move(&game, d)) {
            game.score += 10;
        }

        if (a > 0 && a < 19 && b > 0 && b < 19) {
            game.foodr = a;
            game.foodc = b;
            game.grid[a][b] = 'F';
            game.planlen = 0;
            game.planpos = 0;
            game.plantargetr = -1;
            game.plantargetc = -1;
        }
    }
}

/* 返回指定方向的行偏移。 */
static int dir_row_delta(int dir)
{
    switch (dir) {
        case 0:
            return -1;
        case 1:
            return 0;
        case 2:
            return 1;
        case 3:
            return 0;
        default:
            return 0;
    }
}

/* 返回指定方向的列偏移。 */
static int dir_col_delta(int dir)
{
    switch (dir) {
        case 0:
            return 0;
        case 1:
            return -1;
        case 2:
            return 0;
        case 3:
            return 1;
        default:
            return 0;
    }
}

/* 压入 A* 最小堆。 */
static void hp_push(Game *game, int r, int c, int f)
{
    int i;
    HNode nd;

    i = ++game->hpsz;
    nd.r = r;
    nd.c = c;
    nd.f = f;

    while (i > 1 && game->hp[i / 2].f > f) {
        game->hp[i] = game->hp[i / 2];
        i /= 2;
    }
    game->hp[i] = nd;
}

/* 弹出 A* 最小堆堆顶。 */
static HNode hp_pop(Game *game)
{
    HNode top;
    HNode last;
    int i;
    int ch;

    top = game->hp[1];
    last = game->hp[game->hpsz--];
    i = 1;

    while (i * 2 <= game->hpsz) {
        ch = i * 2;
        if (ch + 1 <= game->hpsz &&
            game->hp[ch + 1].f < game->hp[ch].f) {
            ch++;
        }
        if (last.f <= game->hp[ch].f) {
            break;
        }
        game->hp[i] = game->hp[ch];
        i = ch;
    }
    game->hp[i] = last;
    return top;
}

/* 计算两点曼哈顿距离。 */
static int mhdist(int r1, int c1, int r2, int c2)
{
    int dr;
    int dc;

    dr = r1 - r2;
    dc = c1 - c2;
    if (dr < 0) {
        dr = -dr;
    }
    if (dc < 0) {
        dc = -dc;
    }
    return dr + dc;
}

/* 判断当前移动是否增长。 */
static int grow_at(const Game *game, int eating, int move_num)
{
    return (eating || move_num % game->nval == 0) ? 1 : 0;
}

/* 由蛇身数组重建身体位置映射。 */
static void rebuild_bodyid(Game *game, const int sr[], const int sc[],
                           int len)
{
    int i;

    memset(game->bodyid, -1, sizeof(game->bodyid));
    for (i = 0; i < len; i++) {
        game->bodyid[sr[i]][sc[i]] = i;
    }
}

/* 为任意状态建立身体占用表。 */
static void build_occ_map(const int sr[], const int sc[], int len,
                          int occ[ROWS][COLS])
{
    int i;

    memset(occ, -1, sizeof(int) * ROWS * COLS);
    for (i = 0; i < len; i++) {
        occ[sr[i]][sc[i]] = i;
    }
}

/* 读取初始地图与 N。 */
static void read_input(Game *game)
{
    int i;

    for (i = 0; i < ROWS; i++) {
        scanf("%s", game->grid[i]);
    }
    scanf("%d", &game->nval);
}

/* 解析初始地图，恢复蛇、食物和障碍状态。 */
static void init_game(Game *game)
{
    int i;
    int j;
    int pr;
    int pc;
    int cr;
    int cc;

    game->srow[0] = -1;
    game->scol[0] = -1;

    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (game->grid[i][j] == 'H') {
                game->srow[0] = i;
                game->scol[0] = j;
            }
        }
    }

    game->slen = 1;
    pr = -1;
    pc = -1;
    cr = game->srow[0];
    cc = game->scol[0];
    while (game->slen < MAXSN) {
        int found;

        found = 0;
        for (i = 0; i < 4; i++) {
            int nr;
            int nc;

            nr = cr + dir_row_delta(i);
            nc = cc + dir_col_delta(i);
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (nr == pr && nc == pc) {
                continue;
            }
            if (game->grid[nr][nc] != 'B') {
                continue;
            }

            game->srow[game->slen] = nr;
            game->scol[game->slen] = nc;
            game->slen++;

            pr = cr;
            pc = cc;
            cr = nr;
            cc = nc;
            found = 1;
            break;
        }
        if (!found) {
            break;
        }
    }
    rebuild_bodyid(game, game->srow, game->scol, game->slen);

    game->foodr = -1;
    game->foodc = -1;
    memset(game->wallmap, 0, sizeof(game->wallmap));
    memset(game->visitcnt, 0, sizeof(game->visitcnt));
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (game->grid[i][j] == 'F') {
                game->foodr = i;
                game->foodc = j;
            }
            if (game->grid[i][j] == '#' || game->grid[i][j] == 'O') {
                game->wallmap[i][j] = 1;
            }
        }
    }
    build_static_dead_map(game);
    game->visitcnt[game->srow[0]][game->scol[0]] = 1;
}

/* 在任意状态上执行 A* 搜索。 */
static int astar_state(Game *game, const int sr[], const int sc[],
                       int len, int step0, int dir0,
                       int tgtr, int tgtc,
                       int pathr[], int pathc[], int *fdir)
{
    int occ[ROWS][COLS];
    int gs[ROWS][COLS];
    int from[ROWS][COLS];
    int closed[ROWS][COLS];
    int hr;
    int hc;
    int plen;
    int d;
    int i;

    *fdir = -1;
    if (tgtr < 0) {
        return 0;
    }

    hr = sr[0];
    hc = sc[0];
    if (hr == tgtr && hc == tgtc) {
        return 0;
    }

    build_occ_map(sr, sc, len, occ);
    memset(gs, 0x3f, sizeof(gs));
    memset(closed, 0, sizeof(closed));
    game->hpsz = 0;

    gs[hr][hc] = 0;
    hp_push(game, hr, hc, mhdist(hr, hc, tgtr, tgtc));

    while (game->hpsz > 0) {
        HNode cur;
        int r;
        int c;

        cur = hp_pop(game);
        r = cur.r;
        c = cur.c;

        if (closed[r][c]) {
            continue;
        }
        closed[r][c] = 1;

        if (r == tgtr && c == tgtc) {
            plen = 0;
            while (r != hr || c != hc) {
                pathr[plen] = r;
                pathc[plen] = c;
                d = from[r][c];
                r -= dir_row_delta(d);
                c -= dir_col_delta(d);
                plen++;
            }

            for (i = 0; i < plen / 2; i++) {
                int t;

                t = pathr[i];
                pathr[i] = pathr[plen - 1 - i];
                pathr[plen - 1 - i] = t;

                t = pathc[i];
                pathc[i] = pathc[plen - 1 - i];
                pathc[plen - 1 - i] = t;
            }

            for (d = 0; d < 4; d++) {
                if (hr + dir_row_delta(d) == pathr[0] &&
                    hc + dir_col_delta(d) == pathc[0]) {
                    *fdir = d;
                    break;
                }
            }
            return plen;
        }

        for (d = 0; d < 4; d++) {
            int nr;
            int nc;
            int ng;

            nr = r + dir_row_delta(d);
            nc = c + dir_col_delta(d);
            if (r == hr && c == hc && d == (dir0 ^ 2)) {
                continue;
            }
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (closed[nr][nc] || game->wallmap[nr][nc]) {
                continue;
            }

            if (occ[nr][nc] >= 0) {
                int bi;
                int nd;
                int gw;

                bi = occ[nr][nc];
                nd = gs[r][c] + 1;
                gw = (step0 + nd) / game->nval - step0 / game->nval;
                if (nd < len + gw - bi) {
                    continue;
                }
            }

            ng = gs[r][c] + 1;
            if (ng < gs[nr][nc]) {
                gs[nr][nc] = ng;
                from[nr][nc] = d;
                hp_push(game, nr, nc,
                        ng + mhdist(nr, nc, tgtr, tgtc));
            }
        }
    }
    return 0;
}

/* 在任意状态上沿指定路径完整模拟移动。 */
static int sim_follow_state(Game *game, const int in_sr[],
                            const int in_sc[], int in_len, int step0,
                            int food_tr, int food_tc,
                            int pathr[], int pathc[], int plen,
                            int out_sr[], int out_sc[],
                            int *out_len, int *out_dir)
{
    int occ[ROWS][COLS];
    int len;
    int i;
    int step;

    for (i = 0; i < in_len; i++) {
        out_sr[i] = in_sr[i];
        out_sc[i] = in_sc[i];
    }
    len = in_len;

    memset(occ, 0, sizeof(occ));
    for (i = 0; i < len; i++) {
        occ[out_sr[i]][out_sc[i]] = 1;
    }

    *out_dir = -1;
    for (step = 0; step < plen; step++) {
        int nr;
        int nc;
        int pr;
        int pc;
        int eating;
        int growing;

        nr = pathr[step];
        nc = pathc[step];
        pr = (step == 0) ? in_sr[0] : pathr[step - 1];
        pc = (step == 0) ? in_sc[0] : pathc[step - 1];
        eating = (step == plen - 1 &&
                  nr == food_tr && nc == food_tc);
        growing = grow_at(game, eating, step0 + step + 1);

        if (!growing) {
            occ[out_sr[len - 1]][out_sc[len - 1]] = 0;
        }
        if (game->wallmap[nr][nc] || occ[nr][nc]) {
            return 0;
        }

        occ[nr][nc] = 1;
        if (growing) {
            for (i = len; i > 0; i--) {
                out_sr[i] = out_sr[i - 1];
                out_sc[i] = out_sc[i - 1];
            }
            len++;
        } else {
            for (i = len - 1; i > 0; i--) {
                out_sr[i] = out_sr[i - 1];
                out_sc[i] = out_sc[i - 1];
            }
        }
        out_sr[0] = nr;
        out_sc[0] = nc;

        for (i = 0; i < 4; i++) {
            if (pr + dir_row_delta(i) == nr &&
                pc + dir_col_delta(i) == nc) {
                *out_dir = i;
                break;
            }
        }
    }

    *out_len = len;
    return 1;
}

/* 统计头部连通块中的可达空间。 */
static int state_space(const Game *game, const int sr[],
                       const int sc[], int len)
{
    int blk[ROWS][COLS];
    int vis[ROWS][COLS];
    int qr[ROWS * COLS];
    int qc[ROWS * COLS];
    int front;
    int back;
    int cnt;
    int i;
    int d;

    memcpy(blk, game->wallmap, sizeof(blk));
    for (i = 0; i < len; i++) {
        blk[sr[i]][sc[i]] = 1;
    }
    blk[sr[0]][sc[0]] = 0;

    memset(vis, 0, sizeof(vis));
    front = 0;
    back = 0;
    vis[sr[0]][sc[0]] = 1;
    qr[back] = sr[0];
    qc[back] = sc[0];
    back++;
    cnt = 1;

    while (front < back) {
        int r;
        int c;

        r = qr[front];
        c = qc[front];
        front++;

        for (d = 0; d < 4; d++) {
            int nr;
            int nc;

            nr = r + dir_row_delta(d);
            nc = c + dir_col_delta(d);
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (vis[nr][nc] || blk[nr][nc]) {
                continue;
            }
            vis[nr][nc] = 1;
            qr[back] = nr;
            qc[back] = nc;
            back++;
            cnt++;
        }
    }
    return cnt;
}

/* 统计当前状态下一步还有多少合法方向。 */
static int state_options(const Game *game, const int sr[],
                         const int sc[], int len, int dir0, int step0)
{
    int occ[ROWS][COLS];
    int d;
    int cnt;

    build_occ_map(sr, sc, len, occ);
    cnt = 0;

    for (d = 0; d < 4; d++) {
        int nr;
        int nc;
        int growing;

        nr = sr[0] + dir_row_delta(d);
        nc = sc[0] + dir_col_delta(d);
        if (d == (dir0 ^ 2)) {
            continue;
        }
        if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
            continue;
        }
        if (game->wallmap[nr][nc]) {
            continue;
        }

        growing = ((step0 + 1) % game->nval == 0) ? 1 : 0;
        if (occ[nr][nc] >= 0) {
            int istail;

            istail = (occ[nr][nc] == len - 1);
            if (!(istail && !growing)) {
                continue;
            }
        }
        cnt++;
    }
    return cnt;
}

/* 统计头部周围被封堵的程度。 */
static int state_wall_touch(const Game *game, const int sr[],
                            const int sc[], int len)
{
    int occ[ROWS][COLS];
    int d;
    int cnt;

    build_occ_map(sr, sc, len, occ);
    occ[sr[0]][sc[0]] = -1;
    cnt = 0;

    for (d = 0; d < 4; d++) {
        int nr;
        int nc;

        nr = sr[0] + dir_row_delta(d);
        nc = sc[0] + dir_col_delta(d);
        if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
            cnt++;
        } else if (game->wallmap[nr][nc] || occ[nr][nc] >= 0) {
            cnt++;
        }
    }
    return cnt;
}

/* 统计某格在静态地图上的出口数，用来识别必死胡同食物。 */
static int static_open_degree(const Game *game, int r, int c)
{
    int d;
    int cnt;

    cnt = 0;
    for (d = 0; d < 4; d++) {
        int nr;
        int nc;

        nr = r + dir_row_delta(d);
        nc = c + dir_col_delta(d);
        if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
            continue;
        }
        if (!game->wallmap[nr][nc]) {
            cnt++;
        }
    }
    return cnt;
}

/* 预计算静态地图中所有树状死胡同格。 */
static void build_static_dead_map(Game *game)
{
    int degree[ROWS][COLS];
    int removed[ROWS][COLS];
    int qr[ROWS * COLS];
    int qc[ROWS * COLS];
    int front;
    int back;
    int i;
    int j;
    int d;

    memset(degree, 0, sizeof(degree));
    memset(removed, 0, sizeof(removed));
    front = 0;
    back = 0;

    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (game->wallmap[i][j]) {
                removed[i][j] = 1;
                continue;
            }
            for (d = 0; d < 4; d++) {
                int nr;
                int nc;

                nr = i + dir_row_delta(d);
                nc = j + dir_col_delta(d);
                if (nr >= 0 && nr < ROWS &&
                    nc >= 0 && nc < COLS &&
                    !game->wallmap[nr][nc]) {
                    degree[i][j]++;
                }
            }
        }
    }

    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (!removed[i][j] && degree[i][j] <= 1) {
                removed[i][j] = 1;
                qr[back] = i;
                qc[back] = j;
                back++;
            }
        }
    }

    while (front < back) {
        int cr;
        int cc;

        cr = qr[front];
        cc = qc[front];
        front++;
        for (d = 0; d < 4; d++) {
            int nr;
            int nc;

            nr = cr + dir_row_delta(d);
            nc = cc + dir_col_delta(d);
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (removed[nr][nc]) {
                continue;
            }
            degree[nr][nc]--;
            if (degree[nr][nc] <= 1) {
                removed[nr][nc] = 1;
                qr[back] = nr;
                qc[back] = nc;
                back++;
            }
        }
    }

    memcpy(game->deadmap, removed, sizeof(game->deadmap));
}

/* 判断食物是否位于静态地图的树状死胡同分支中。 */
static int static_dead_cell(const Game *game, int r, int c)
{
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS) {
        return 0;
    }
    return game->deadmap[r][c] ? 1 : 0;
}

/* 返回程序启动以来的大致 CPU 毫秒数。 */
static int elapsed_ms(const Game *game)
{
    clock_t used;

    used = clock() - game->start_clock;
    return (int)(used * 1000 / CLOCKS_PER_SEC);
}

/* 在真实全局状态上模拟整条路径。 */
static int sim_follow_global(Game *game, int pathr[], int pathc[],
                             int plen)
{
    int lastdir;

    return sim_follow_state(game, game->srow, game->scol, game->slen,
                            game->stepcnt, game->foodr, game->foodc,
                            pathr, pathc, plen,
                            game->tmp_sr, game->tmp_sc,
                            &game->tmp_slen, &lastdir);
}

/* 检查模拟后是否仍有足够空间并能接上尾部。 */
static int sim_safe_like(const Game *game, int min_space)
{
    int blk[ROWS][COLS];
    int vis[ROWS][COLS];
    int qr[ROWS * COLS];
    int qc[ROWS * COLS];
    int front;
    int back;
    int cnt;
    int d;
    int i;
    int tok;
    int tr;
    int tc;

    memcpy(blk, game->wallmap, sizeof(blk));
    for (i = 0; i < game->tmp_slen; i++) {
        blk[game->tmp_sr[i]][game->tmp_sc[i]] = 1;
    }
    blk[game->tmp_sr[0]][game->tmp_sc[0]] = 0;

    memset(vis, 0, sizeof(vis));
    front = 0;
    back = 0;
    vis[game->tmp_sr[0]][game->tmp_sc[0]] = 1;
    qr[back] = game->tmp_sr[0];
    qc[back] = game->tmp_sc[0];
    back++;
    cnt = 1;

    while (front < back) {
        int r;
        int c;

        r = qr[front];
        c = qc[front];
        front++;

        for (d = 0; d < 4; d++) {
            int nr;
            int nc;

            nr = r + dir_row_delta(d);
            nc = c + dir_col_delta(d);
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (vis[nr][nc] || blk[nr][nc]) {
                continue;
            }
            vis[nr][nc] = 1;
            qr[back] = nr;
            qc[back] = nc;
            back++;
            cnt++;
        }
    }

    tok = 0;
    tr = game->tmp_sr[game->tmp_slen - 1];
    tc = game->tmp_sc[game->tmp_slen - 1];
    for (d = 0; d < 4; d++) {
        int nr;
        int nc;

        nr = tr + dir_row_delta(d);
        nc = tc + dir_col_delta(d);
        if (nr >= 0 && nr < ROWS &&
            nc >= 0 && nc < COLS &&
            vis[nr][nc]) {
            tok = 1;
        }
    }

    if (game->nval <= 1) {
        return cnt >= min_space;
    }
    return tok && cnt >= min_space;
}

/* 单步落点后的连通块评估，用于低 N 策略。 */
static int flood_eval(const Game *game, int startr, int startc,
                      int blk[ROWS][COLS], int tailr, int tailc,
                      int *tail_ok)
{
    int vis[ROWS][COLS];
    int qr[ROWS * COLS];
    int qc[ROWS * COLS];
    int front;
    int back;
    int cnt;
    int d;

    (void)game;
    memset(vis, 0, sizeof(vis));
    front = 0;
    back = 0;
    vis[startr][startc] = 1;
    qr[back] = startr;
    qc[back] = startc;
    back++;
    cnt = 1;

    while (front < back) {
        int r;
        int c;

        r = qr[front];
        c = qc[front];
        front++;

        for (d = 0; d < 4; d++) {
            int nr;
            int nc;

            nr = r + dir_row_delta(d);
            nc = c + dir_col_delta(d);
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (vis[nr][nc] || blk[nr][nc]) {
                continue;
            }
            vis[nr][nc] = 1;
            qr[back] = nr;
            qc[back] = nc;
            back++;
            cnt++;
        }
    }

    *tail_ok = 0;
    for (d = 0; d < 4; d++) {
        int nr;
        int nc;

        nr = tailr + dir_row_delta(d);
        nc = tailc + dir_col_delta(d);
        if (nr >= 0 && nr < ROWS &&
            nc >= 0 && nc < COLS &&
            vis[nr][nc]) {
            *tail_ok = 1;
        }
    }
    return cnt;
}

/* 低 N 时更强调短期安全和追尾保活。 */
static int choose_direction_low(Game *game)
{
    int d;
    int bestd;
    int bestval;
    int pathr[ROWS * COLS];
    int pathc[ROWS * COLS];
    int plen;
    int fdir;
    int tdir;
    int safe_food_dir;
    int tail_chase_dir;
    int threshold;
    int extra;

    threshold = (game->nval <= 1) ? 10 : 6;
    safe_food_dir = -1;
    fdir = -1;

    plen = astar_state(game, game->srow, game->scol, game->slen,
                       game->stepcnt, game->curdir,
                       game->foodr, game->foodc,
                       pathr, pathc, &fdir);
    if (plen > 0 && fdir >= 0) {
        if (sim_follow_global(game, pathr, pathc, plen)) {
            extra = (plen > 10) ? plen / 2 : 0;
            if (sim_safe_like(game, threshold + extra)) {
                safe_food_dir = fdir;
            }
        }
        if (safe_food_dir < 0 &&
            static_dead_cell(game, game->foodr, game->foodc) &&
            sim_follow_global(game, pathr, pathc, plen)) {
            return fdir;
        }
    }

    tail_chase_dir = -1;
    if (game->nval > 1) {
        tdir = -1;
        astar_state(game, game->srow, game->scol, game->slen,
                    game->stepcnt, game->curdir,
                    game->srow[game->slen - 1],
                    game->scol[game->slen - 1],
                    pathr, pathc, &tdir);
        if (tdir >= 0) {
            tail_chase_dir = tdir;
        }
    }

    bestd = -1;
    bestval = -1;
    for (d = 0; d < 4; d++) {
        int nr;
        int nc;
        int eating;
        int growing;
        int blk[ROWS][COLS];
        int tailr;
        int tailc;
        int tok;
        int ff;
        int val;
        int fdist;
        int i;

        nr = game->srow[0] + dir_row_delta(d);
        nc = game->scol[0] + dir_col_delta(d);
        if (d == (game->curdir ^ 2)) {
            continue;
        }
        if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
            continue;
        }
        if (game->wallmap[nr][nc]) {
            continue;
        }

        eating = (nr == game->foodr && nc == game->foodc) ? 1 : 0;
        growing = grow_at(game, eating, game->stepcnt + 1);
        if (game->bodyid[nr][nc] >= 0) {
            int istail;

            istail = (nr == game->srow[game->slen - 1] &&
                      nc == game->scol[game->slen - 1]);
            if (!(istail && !growing)) {
                continue;
            }
        }

        memcpy(blk, game->wallmap, sizeof(blk));
        for (i = 0; i < game->slen - 1; i++) {
            blk[game->srow[i]][game->scol[i]] = 1;
        }
        if (growing) {
            blk[game->srow[game->slen - 1]]
               [game->scol[game->slen - 1]] = 1;
        }
        blk[nr][nc] = 0;

        if (growing) {
            tailr = game->srow[game->slen - 1];
            tailc = game->scol[game->slen - 1];
        } else {
            tailr = game->srow[game->slen - 2];
            tailc = game->scol[game->slen - 2];
        }

        ff = flood_eval(game, nr, nc, blk, tailr, tailc, &tok);
        fdist = (game->foodr >= 0) ?
                mhdist(nr, nc, game->foodr, game->foodc) : 40;

        val = ff;
        if (game->nval > 1 && tok) {
            val += 5000;
        }
        if (d == safe_food_dir) {
            val += (game->nval <= 1) ? 6000 : 10000;
        }
        if (eating && (game->nval <= 1 || tok)) {
            val += (game->nval <= 1) ? 5000 : 8000;
        } else if (eating) {
            val += 2000;
        }
        if (d == fdir && safe_food_dir < 0 && tok) {
            val += 1500;
        } else if (d == fdir && safe_food_dir < 0) {
            val += 300;
        }
        if (d == tail_chase_dir && tok) {
            val += 2500;
        } else if (d == tail_chase_dir) {
            val += 500;
        }
        if (game->nval <= 1) {
            val += (40 - fdist) * 4;
            val -= game->visitcnt[nr][nc] * 120;
        } else {
            val += (40 - fdist) * 10;
            val -= game->visitcnt[nr][nc] * 40;
        }

        if (val > bestval) {
            bestval = val;
            bestd = d;
        }
    }

    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            int eating;
            int growing;
            int dummy_len;
            int dummy_sr[MAXSN];
            int dummy_sc[MAXSN];

            if (sim_one_step(game, d, dummy_sr, dummy_sc,
                             &dummy_len, &eating, &growing)) {
                bestd = d;
                break;
            }
        }
    }
    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            if (d != (game->curdir ^ 2)) {
                bestd = d;
                break;
            }
        }
    }
    return bestd;
}

/* 判断吃到路径终点后局面是否仍可控。 */
static int safe_after_path(Game *game, const int in_sr[],
                           const int in_sc[], int in_len, int step0,
                           int food_tr, int food_tc,
                           int pathr[], int pathc[], int plen)
{
    int backr[ROWS * COLS];
    int backc[ROWS * COLS];
    int end_step;
    int lastdir;
    int space;
    int opts;
    int need;
    int dummy;
    int tail_len;

    if (plen <= 0) {
        return 0;
    }
    if (!sim_follow_state(game, in_sr, in_sc, in_len, step0,
                          food_tr, food_tc,
                          pathr, pathc, plen,
                          game->tmp_sr, game->tmp_sc,
                          &game->tmp_slen, &lastdir)) {
        return 0;
    }
    if (lastdir < 0) {
        return 0;
    }

    end_step = step0 + plen;
    space = state_space(game, game->tmp_sr, game->tmp_sc,
                        game->tmp_slen);
    opts = state_options(game, game->tmp_sr, game->tmp_sc,
                         game->tmp_slen, lastdir, end_step);

    if (game->nval <= 1) {
        need = 6 + plen / 2;
        if (game->tmp_slen > 200) {
            need += 2;
        }
        return space >= need && opts > 0;
    }

    dummy = -1;
    tail_len = astar_state(game, game->tmp_sr, game->tmp_sc,
                           game->tmp_slen, end_step, lastdir,
                           game->tmp_sr[game->tmp_slen - 1],
                           game->tmp_sc[game->tmp_slen - 1],
                           backr, backc, &dummy);
    if (tail_len <= 0) {
        return 0;
    }

    if (game->nval <= 2) {
        need = 6;
    } else if (game->nval <= 8) {
        need = 5;
    } else {
        need = 4;
    }
    if (plen > 12) {
        need += plen / 6;
    }
    if (game->tmp_slen > 220) {
        need += 2;
    }
    if (space < need) {
        return 0;
    }
    if (opts <= 0 && tail_len > 1) {
        return 0;
    }
    return 1;
}

/* 仅模拟一步，作为所有评分逻辑的共同入口。 */
static int sim_one_step(const Game *game, int d, int out_sr[],
                        int out_sc[], int *out_len,
                        int *eating_out, int *growing_out)
{
    int nr;
    int nc;
    int eating;
    int growing;
    int i;
    int len;

    if (d == (game->curdir ^ 2)) {
        return 0;
    }

    nr = game->srow[0] + dir_row_delta(d);
    nc = game->scol[0] + dir_col_delta(d);
    if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
        return 0;
    }
    if (game->wallmap[nr][nc]) {
        return 0;
    }

    eating = (nr == game->foodr && nc == game->foodc) ? 1 : 0;
    growing = grow_at(game, eating, game->stepcnt + 1);
    if (game->bodyid[nr][nc] >= 0) {
        int istail;

        istail = (nr == game->srow[game->slen - 1] &&
                  nc == game->scol[game->slen - 1]);
        if (!(istail && !growing)) {
            return 0;
        }
    }

    for (i = 0; i < game->slen; i++) {
        out_sr[i] = game->srow[i];
        out_sc[i] = game->scol[i];
    }

    len = game->slen;
    if (growing) {
        for (i = len; i > 0; i--) {
            out_sr[i] = out_sr[i - 1];
            out_sc[i] = out_sc[i - 1];
        }
        len++;
    } else {
        for (i = len - 1; i > 0; i--) {
            out_sr[i] = out_sr[i - 1];
            out_sc[i] = out_sc[i - 1];
        }
    }

    out_sr[0] = nr;
    out_sc[0] = nc;
    *out_len = len;
    *eating_out = eating;
    *growing_out = growing;
    return 1;
}

/* 把相邻目标格转换为方向编号。 */
static int direction_to_cell(const Game *game, int r, int c)
{
    int d;

    for (d = 0; d < 4; d++) {
        if (game->srow[0] + dir_row_delta(d) == r &&
            game->scol[0] + dir_col_delta(d) == c) {
            return d;
        }
    }
    return -1;
}

/* 如果上一轮安全路径仍然有效，直接沿路径前进。 */
static int follow_cached_plan(Game *game)
{
    int d;
    int nsr[MAXSN];
    int nsc[MAXSN];
    int nlen;
    int eating;
    int growing;

    if (game->planpos >= game->planlen) {
        return -1;
    }
    if (game->plantargetr != game->foodr ||
        game->plantargetc != game->foodc) {
        return -1;
    }

    d = direction_to_cell(game, game->planr[game->planpos],
                          game->planc[game->planpos]);
    if (d < 0) {
        return -1;
    }
    if (!sim_one_step(game, d, nsr, nsc, &nlen,
                      &eating, &growing)) {
        return -1;
    }
    game->planpos++;
    return d;
}

/* 高 N 和临近时间上限时使用单步轻量评分。 */
static int choose_direction_fast(Game *game)
{
    int d;
    int bestd;
    int bestval;
    int nsr[MAXSN];
    int nsc[MAXSN];
    int nlen;

    bestd = -1;
    bestval = -1000000000;
    for (d = 0; d < 4; d++) {
        int eating;
        int growing;
        int space;
        int opts;
        int wall_touch;
        int fdist;
        int val;

        if (!sim_one_step(game, d, nsr, nsc, &nlen,
                          &eating, &growing)) {
            continue;
        }

        space = state_space(game, nsr, nsc, nlen);
        opts = state_options(game, nsr, nsc, nlen,
                             d, game->stepcnt + 1);
        wall_touch = state_wall_touch(game, nsr, nsc, nlen);
        fdist = (game->foodr >= 0) ?
                mhdist(nsr[0], nsc[0], game->foodr, game->foodc) : 40;

        val = space * 55 + opts * 500 - wall_touch * 160;
        val += (40 - fdist) * 55;
        val -= game->visitcnt[nsr[0]][nsc[0]] *
               ((game->nval >= 128) ? 260 : 170);
        if (space <= 4) {
            val -= 20000;
        } else if (space <= 8) {
            val -= 5000;
        }
        if (opts <= 0) {
            val -= 16000;
        } else if (opts == 1) {
            val -= 1800;
        }
        if (eating) {
            val += 16000;
        }
        if (growing && opts <= 1) {
            val -= 2500;
        }
        if (game->stepcnt - game->last_food_step > 80) {
            val += (40 - fdist) * 25;
        }

        if (val > bestval) {
            bestval = val;
            bestd = d;
        }
    }

    if (bestd >= 0) {
        return bestd;
    }
    return choose_finish_direction(game);
}

/* 时间预算紧张时选择尽快撞墙或撞身结束。 */
static int choose_finish_direction(const Game *game)
{
    int d;
    int bestd;
    int bestdist;

    bestd = -1;
    bestdist = 1000;
    for (d = 0; d < 4; d++) {
        int r;
        int c;
        int dist;

        if (d == (game->curdir ^ 2)) {
            continue;
        }
        r = game->srow[0] + dir_row_delta(d);
        c = game->scol[0] + dir_col_delta(d);
        dist = 1;
        while (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
            if (game->wallmap[r][c] || game->bodyid[r][c] >= 0) {
                break;
            }
            r += dir_row_delta(d);
            c += dir_col_delta(d);
            dist++;
        }
        if (dist < bestdist) {
            bestdist = dist;
            bestd = d;
        }
    }

    if (bestd >= 0) {
        return bestd;
    }
    for (d = 0; d < 4; d++) {
        if (d != (game->curdir ^ 2)) {
            return d;
        }
    }
    return game->curdir;
}

/* 根据 N 的权重和已得分决定是否主动收尾，避免 400ms 超时。 */
static int should_finish_fast(const Game *game, int elapsed)
{
    if (elapsed >= 330) {
        return 1;
    }
    if (elapsed >= 300) {
        return game->score >= 40;
    }
    if (game->nval >= 512) {
        return game->stepcnt >= 5200 && game->score >= 1500;
    }
    if (game->nval >= 256) {
        return game->stepcnt >= 5600 && game->score >= 1600;
    }
    if (game->nval >= 128) {
        return game->stepcnt >= 6200 && game->score >= 1600;
    }
    if (game->nval >= 64) {
        return game->stepcnt >= 6800 && game->score >= 1400;
    }
    return 0;
}

/* 高 N 时使用更重的前瞻评分。 */
static int choose_direction(Game *game)
{
    int bestd;
    int bestval;
    int d;
    int pathr[ROWS * COLS];
    int pathc[ROWS * COLS];
    int nsr[MAXSN];
    int nsc[MAXSN];
    int nlen;
    int fill_level;
    int hunger;
    int elapsed;

    fill_level = game->slen * 100 / 314;
    hunger = game->stepcnt - game->last_food_step;
    elapsed = elapsed_ms(game);

    if (should_finish_fast(game, elapsed)) {
        return choose_finish_direction(game);
    }

    if (game->nval <= 4) {
        return choose_direction_low(game);
    }
    if ((game->nval >= 512 && game->stepcnt > 7000) ||
        (game->nval >= 256 && elapsed > 320) ||
        (game->nval >= 128 && elapsed > 340)) {
        return choose_direction_fast(game);
    }

    if (game->nval >= 128) {
        d = follow_cached_plan(game);
        if (d >= 0) {
            return d;
        }
    }

    if (game->foodr >= 0) {
        int fdir;
        int food_len;

        fdir = -1;
        food_len = astar_state(game, game->srow, game->scol,
                               game->slen, game->stepcnt,
                               game->curdir, game->foodr,
                               game->foodc, pathr, pathc, &fdir);
        if (food_len > 0 && fdir >= 0 &&
            safe_after_path(game, game->srow, game->scol,
                            game->slen, game->stepcnt,
                            game->foodr, game->foodc,
                            pathr, pathc, food_len)) {
            int direct_need;
            int direct_space;

            if (game->nval <= 16) {
                direct_need = 10;
            } else if (game->nval <= 64) {
                direct_need = 12;
            } else {
                direct_need = 8;
            }
            direct_need += food_len / 12;
            direct_space = state_space(game, game->tmp_sr,
                                       game->tmp_sc, game->tmp_slen);
            if (direct_space >= direct_need) {
                game->planlen = food_len;
                game->planpos = 1;
                game->plantargetr = game->foodr;
                game->plantargetc = game->foodc;
                memcpy(game->planr, pathr, sizeof(int) * food_len);
                memcpy(game->planc, pathc, sizeof(int) * food_len);
                return fdir;
            }
        }

        /* 食物若在静态死胡同中，吃后必死，尽快吃掉避免空转。 */
        if (food_len > 0 && fdir >= 0 &&
            (static_open_degree(game, game->foodr, game->foodc) <= 1 ||
             static_dead_cell(game, game->foodr, game->foodc)) &&
            sim_follow_global(game, pathr, pathc, food_len)) {
            game->planlen = food_len;
            game->planpos = 1;
            game->plantargetr = game->foodr;
            game->plantargetc = game->foodc;
            memcpy(game->planr, pathr, sizeof(int) * food_len);
            memcpy(game->planc, pathc, sizeof(int) * food_len);
            return fdir;
        }

    }

    bestd = -1;
    bestval = -1000000000;

    for (d = 0; d < 4; d++) {
        int eating;
        int growing;
        int space;
        int opts;
        int wall_touch;
        int tail_len;
        int food_len;
        int food_safe;
        int dummy;
        int fdist;
        int val;

        if (!sim_one_step(game, d, nsr, nsc, &nlen,
                          &eating, &growing)) {
            continue;
        }

        space = state_space(game, nsr, nsc, nlen);
        opts = state_options(game, nsr, nsc, nlen,
                             d, game->stepcnt + 1);
        wall_touch = state_wall_touch(game, nsr, nsc, nlen);

        tail_len = -1;
        if (game->nval > 1) {
            dummy = -1;
            tail_len = astar_state(game, nsr, nsc, nlen,
                                   game->stepcnt + 1, d,
                                   nsr[nlen - 1], nsc[nlen - 1],
                                   pathr, pathc, &dummy);
        }

        food_len = 0;
        food_safe = 0;
        if (!eating && game->foodr >= 0) {
            dummy = -1;
            food_len = astar_state(game, nsr, nsc, nlen,
                                   game->stepcnt + 1, d,
                                   game->foodr, game->foodc,
                                   pathr, pathc, &dummy);
            if (food_len > 0) {
                food_safe = safe_after_path(game, nsr, nsc, nlen,
                                            game->stepcnt + 1,
                                            game->foodr, game->foodc,
                                            pathr, pathc, food_len);
            }
        }

        fdist = (game->foodr >= 0) ?
                mhdist(nsr[0], nsc[0], game->foodr, game->foodc) : 40;
        val = 0;

        val += space * 40;
        val += opts * 500;
        val -= wall_touch * 120;

        if (space <= 4) {
            val -= 15000;
        } else if (space <= 8) {
            val -= 4000;
        }
        if (opts <= 0) {
            val -= 12000;
        } else if (opts == 1) {
            val -= 1200;
        }

        if (tail_len > 0) {
            val += 11000 + (tail_len < 80 ? tail_len : 80) * 25;
        } else {
            val -= 3000;
        }

        if (food_safe) {
            val += 24000 - food_len *
                   ((fill_level > 60) ? 90 : 45);
        } else if (food_len > 0) {
            val += 2200 - food_len * 12;
        }

        if (eating) {
            if (tail_len > 0 && space >= 6) {
                val += 12000;
            } else {
                val += 3000;
            }
        }

        if (fill_level < 35 && food_safe) {
            val += 2000;
        }
        if (fill_level > 70 && tail_len > 0) {
            val += 2000;
        }
        if (growing && opts <= 1) {
            val -= 2000;
        }

        val += (40 - fdist) * 12;
        val -= game->visitcnt[nsr[0]][nsc[0]] *
               ((game->nval <= 16) ? 70 : 220);
        if (hunger > 40 && food_len > 0) {
            val += (hunger - 40) * (food_safe ? 120 : 45);
        }
        if (hunger > 100 && eating) {
            val += 6000;
        }
        if (hunger > 140 && food_len <= 0 && !eating) {
            val -= 4000;
        }

        if (val > bestval) {
            bestval = val;
            bestd = d;
        }
    }

    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            int eating;
            int growing;

            if (sim_one_step(game, d, nsr, nsc, &nlen,
                             &eating, &growing)) {
                bestd = d;
                break;
            }
        }
    }
    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            if (d != (game->curdir ^ 2)) {
                bestd = d;
                break;
            }
        }
    }
    return bestd;
}

/* 在真实状态上执行一步移动。 */
static int do_move(Game *game, int d)
{
    int i;
    int nr;
    int nc;
    int eating;
    int growing;
    int otailr;
    int otailc;

    nr = game->srow[0] + dir_row_delta(d);
    nc = game->scol[0] + dir_col_delta(d);
    eating = (nr == game->foodr && nc == game->foodc) ? 1 : 0;
    growing = grow_at(game, eating, game->stepcnt + 1);
    otailr = game->srow[game->slen - 1];
    otailc = game->scol[game->slen - 1];

    if (growing) {
        for (i = game->slen; i > 0; i--) {
            game->srow[i] = game->srow[i - 1];
            game->scol[i] = game->scol[i - 1];
        }
        game->slen++;
    } else {
        for (i = game->slen - 1; i > 0; i--) {
            game->srow[i] = game->srow[i - 1];
            game->scol[i] = game->scol[i - 1];
        }
        game->grid[otailr][otailc] = '.';
    }

    game->grid[game->srow[1]][game->scol[1]] = 'B';
    game->srow[0] = nr;
    game->scol[0] = nc;
    game->grid[nr][nc] = 'H';

    rebuild_bodyid(game, game->srow, game->scol, game->slen);
    game->curdir = d;
    game->stepcnt++;
    game->visitcnt[nr][nc]++;

    if (eating) {
        game->foodr = -1;
        game->foodc = -1;
        game->last_food_step = game->stepcnt;
        game->planlen = 0;
        game->planpos = 0;
        game->plantargetr = -1;
        game->plantargetc = -1;
    }
    return eating;
}

/* 按题目要求输出最终地图和得分。 */
static void print_final(const Game *game)
{
    int i;

    for (i = 0; i < ROWS; i++) {
        printf("%s\n", game->grid[i]);
    }
    printf("%d\n", game->score);
    fflush(stdout);
}
