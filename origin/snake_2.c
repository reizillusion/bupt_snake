/*
 * 文件: snake.c
 * 说明: 贪吃蛇交互程序。
 *       本文件保留原有的 A* 搜索、低 N 和高 N 分档决策策略，
 *       仅调整代码风格、注释和文件组织形式，不改变算法行为。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROWS 20
#define COLS 20
#define MAXSN 400
#define HCAP 1601

static const int drow[4] = {-1, 0, 1, 0};
static const int dcol[4] = {0, -1, 0, 1};

typedef struct {
    int r;
    int c;
    int f;
} HNode;

static char grid[ROWS][COLS + 1];
static int srow[MAXSN], scol[MAXSN], slen;
static int bodyid[ROWS][COLS];
static int wallmap[ROWS][COLS];
static int visitcnt[ROWS][COLS];
static int curdir, score, nval, stepcnt;
static int last_food_step;
static int foodr, foodc;

static HNode hp[HCAP];
static int hpsz;

static int tmp_sr[MAXSN], tmp_sc[MAXSN], tmp_slen;

static void hp_push(int r, int c, int f);
static HNode hp_pop(void);
static int mhdist(int r1, int c1, int r2, int c2);
static int grow_at(int eating, int move_num);
static void rebuild_bodyid(const int sr[], const int sc[], int len);
static void build_occ_map(const int sr[], const int sc[], int len,
                          int occ[ROWS][COLS]);
static void read_input(void);
static void init_game(void);
static int astar_state(const int sr[], const int sc[], int len,
                       int step0, int dir0,
                       int tgtr, int tgtc,
                       int pathr[], int pathc[], int *fdir);
static int sim_follow_state(const int in_sr[], const int in_sc[],
                            int in_len, int step0,
                            int food_tr, int food_tc,
                            int pathr[], int pathc[], int plen,
                            int out_sr[], int out_sc[],
                            int *out_len, int *out_dir);
static int state_space(const int sr[], const int sc[], int len);
static int state_options(const int sr[], const int sc[], int len,
                         int dir0, int step0);
static int state_wall_touch(const int sr[], const int sc[], int len);
static int sim_follow_global(int pathr[], int pathc[], int plen);
static int sim_safe_like(int min_space);
static int flood_eval(int startr, int startc, int blk[ROWS][COLS],
                      int tailr, int tailc, int *tail_ok);
static int choose_direction_low(void);
static int safe_after_path(const int in_sr[], const int in_sc[],
                           int in_len, int step0,
                           int food_tr, int food_tc,
                           int pathr[], int pathc[], int plen);
static int sim_one_step(int d, int out_sr[], int out_sc[], int *out_len,
                        int *eating_out, int *growing_out);
static int choose_direction(void);
static int do_move(int d);
static void print_final(void);

int main(void)
{
    int a;
    int b;
    int d;

    read_input();
    init_game();

    score = 0;
    curdir = 0;
    stepcnt = 0;
    last_food_step = 0;

    for (;;) {
        d = choose_direction();
        printf("%c\n%d\n", "WASD"[d], score);
        fflush(stdout);

        scanf("%d %d", &a, &b);
        if (a == 100 && b == 100) {
            print_final();
            return 0;
        }

        if (do_move(d)) {
            score += 10;
        }

        if (a > 0 && a < 19 && b > 0 && b < 19) {
            foodr = a;
            foodc = b;
            grid[a][b] = 'F';
        }
    }
}

/* 压入 A* 最小堆。 */
static void hp_push(int r, int c, int f)
{
    int i;
    HNode nd;

    i = ++hpsz;
    nd.r = r;
    nd.c = c;
    nd.f = f;

    while (i > 1 && hp[i / 2].f > f) {
        hp[i] = hp[i / 2];
        i /= 2;
    }
    hp[i] = nd;
}

/* 弹出 A* 最小堆堆顶。 */
static HNode hp_pop(void)
{
    HNode top;
    HNode last;
    int i;
    int ch;

    top = hp[1];
    last = hp[hpsz--];
    i = 1;

    while (i * 2 <= hpsz) {
        ch = i * 2;
        if (ch + 1 <= hpsz && hp[ch + 1].f < hp[ch].f) {
            ch++;
        }
        if (last.f <= hp[ch].f) {
            break;
        }
        hp[i] = hp[ch];
        i = ch;
    }
    hp[i] = last;
    return top;
}

/* 计算两点的曼哈顿距离。 */
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

/* 根据吃食物和强制增长规则判断本步是否增长。 */
static int grow_at(int eating, int move_num)
{
    return (eating || move_num % nval == 0) ? 1 : 0;
}

/* 由当前蛇身数组重建位置到身体下标的映射。 */
static void rebuild_bodyid(const int sr[], const int sc[], int len)
{
    int i;

    memset(bodyid, -1, sizeof(bodyid));
    for (i = 0; i < len; i++) {
        bodyid[sr[i]][sc[i]] = i;
    }
}

/* 为任意模拟状态建立身体占用表。 */
static void build_occ_map(const int sr[], const int sc[], int len,
                          int occ[ROWS][COLS])
{
    int i;

    memset(occ, -1, sizeof(int) * ROWS * COLS);
    for (i = 0; i < len; i++) {
        occ[sr[i]][sc[i]] = i;
    }
}

/* 读取初始地图和 N。 */
static void read_input(void)
{
    int i;

    for (i = 0; i < ROWS; i++) {
        scanf("%s", grid[i]);
    }
    scanf("%d", &nval);
}

/* 解析初始地图，恢复蛇、食物和障碍状态。 */
static void init_game(void)
{
    int i;
    int j;
    int pr;
    int pc;
    int cr;
    int cc;

    srow[0] = -1;
    scol[0] = -1;

    /* 先找到蛇头。 */
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (grid[i][j] == 'H') {
                srow[0] = i;
                scol[0] = j;
            }
        }
    }

    /* 按相邻关系恢复整条蛇身。 */
    slen = 1;
    pr = -1;
    pc = -1;
    cr = srow[0];
    cc = scol[0];
    while (slen < MAXSN) {
        int found;

        found = 0;
        for (i = 0; i < 4; i++) {
            int nr;
            int nc;

            nr = cr + drow[i];
            nc = cc + dcol[i];
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (nr == pr && nc == pc) {
                continue;
            }
            if (grid[nr][nc] != 'B') {
                continue;
            }

            srow[slen] = nr;
            scol[slen] = nc;
            slen++;

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
    rebuild_bodyid(srow, scol, slen);

    /* 记录食物、障碍和访问计数。 */
    foodr = -1;
    foodc = -1;
    memset(wallmap, 0, sizeof(wallmap));
    memset(visitcnt, 0, sizeof(visitcnt));
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (grid[i][j] == 'F') {
                foodr = i;
                foodc = j;
            }
            if (grid[i][j] == '#' || grid[i][j] == 'O') {
                wallmap[i][j] = 1;
            }
        }
    }
    visitcnt[srow[0]][scol[0]] = 1;
}

/* 在任意状态上执行 A* 搜索。 */
static int astar_state(const int sr[], const int sc[], int len,
                       int step0, int dir0,
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
    hpsz = 0;

    gs[hr][hc] = 0;
    hp_push(hr, hc, mhdist(hr, hc, tgtr, tgtc));

    while (hpsz > 0) {
        HNode cur;
        int r;
        int c;

        cur = hp_pop();
        r = cur.r;
        c = cur.c;

        if (closed[r][c]) {
            continue;
        }
        closed[r][c] = 1;

        /* 找到目标后逆推路径，再翻转成头到目标的顺序。 */
        if (r == tgtr && c == tgtc) {
            plen = 0;
            while (r != hr || c != hc) {
                pathr[plen] = r;
                pathc[plen] = c;
                d = from[r][c];
                r -= drow[d];
                c -= dcol[d];
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
                if (hr + drow[d] == pathr[0] &&
                    hc + dcol[d] == pathc[0]) {
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

            nr = r + drow[d];
            nc = c + dcol[d];

            if (r == hr && c == hc && d == (dir0 ^ 2)) {
                continue;
            }
            if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
                continue;
            }
            if (closed[nr][nc] || wallmap[nr][nc]) {
                continue;
            }

            /* 身体格子只有在未来会腾开时才允许通过。 */
            if (occ[nr][nc] >= 0) {
                int bi;
                int nd;
                int gw;

                bi = occ[nr][nc];
                nd = gs[r][c] + 1;
                gw = (step0 + nd) / nval - step0 / nval;
                if (nd < len + gw - bi) {
                    continue;
                }
            }

            ng = gs[r][c] + 1;
            if (ng < gs[nr][nc]) {
                gs[nr][nc] = ng;
                from[nr][nc] = d;
                hp_push(nr, nc, ng + mhdist(nr, nc, tgtr, tgtc));
            }
        }
    }
    return 0;
}

/* 在任意状态上沿给定路径完整模拟蛇的移动。 */
static int sim_follow_state(const int in_sr[], const int in_sc[],
                            int in_len, int step0,
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
        eating = (step == plen - 1 && nr == food_tr && nc == food_tc);
        growing = grow_at(eating, step0 + step + 1);

        if (!growing) {
            occ[out_sr[len - 1]][out_sc[len - 1]] = 0;
        }
        if (wallmap[nr][nc] || occ[nr][nc]) {
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
            if (pr + drow[i] == nr && pc + dcol[i] == nc) {
                *out_dir = i;
                break;
            }
        }
    }

    *out_len = len;
    return 1;
}

/* 统计当前头部所在连通块的可达空间。 */
static int state_space(const int sr[], const int sc[], int len)
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

    memcpy(blk, wallmap, sizeof(blk));
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

            nr = r + drow[d];
            nc = c + dcol[d];
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

/* 统计当前状态下一步还能合法走多少个方向。 */
static int state_options(const int sr[], const int sc[], int len,
                         int dir0, int step0)
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

        nr = sr[0] + drow[d];
        nc = sc[0] + dcol[d];
        if (d == (dir0 ^ 2)) {
            continue;
        }
        if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
            continue;
        }
        if (wallmap[nr][nc]) {
            continue;
        }

        growing = ((step0 + 1) % nval == 0) ? 1 : 0;
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

/* 统计头部周围被墙、障碍或身体包住的程度。 */
static int state_wall_touch(const int sr[], const int sc[], int len)
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

        nr = sr[0] + drow[d];
        nc = sc[0] + dcol[d];
        if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
            cnt++;
        } else if (wallmap[nr][nc] || occ[nr][nc] >= 0) {
            cnt++;
        }
    }
    return cnt;
}

/* 在真实全局状态上模拟整条路径。 */
static int sim_follow_global(int pathr[], int pathc[], int plen)
{
    int lastdir;

    return sim_follow_state(srow, scol, slen, stepcnt, foodr, foodc,
                            pathr, pathc, plen,
                            tmp_sr, tmp_sc, &tmp_slen, &lastdir);
}

/* 检查模拟后的状态是否仍有足够空间并能接上尾部。 */
static int sim_safe_like(int min_space)
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

    memcpy(blk, wallmap, sizeof(blk));
    for (i = 0; i < tmp_slen; i++) {
        blk[tmp_sr[i]][tmp_sc[i]] = 1;
    }
    blk[tmp_sr[0]][tmp_sc[0]] = 0;

    memset(vis, 0, sizeof(vis));
    front = 0;
    back = 0;
    vis[tmp_sr[0]][tmp_sc[0]] = 1;
    qr[back] = tmp_sr[0];
    qc[back] = tmp_sc[0];
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

            nr = r + drow[d];
            nc = c + dcol[d];
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
    tr = tmp_sr[tmp_slen - 1];
    tc = tmp_sc[tmp_slen - 1];
    for (d = 0; d < 4; d++) {
        int nr;
        int nc;

        nr = tr + drow[d];
        nc = tc + dcol[d];
        if (nr >= 0 && nr < ROWS &&
            nc >= 0 && nc < COLS &&
            vis[nr][nc]) {
            tok = 1;
        }
    }

    if (nval <= 1) {
        return cnt >= min_space;
    }
    return tok && cnt >= min_space;
}

/* 单步落点后的连通块评估，用于低 N 策略。 */
static int flood_eval(int startr, int startc, int blk[ROWS][COLS],
                      int tailr, int tailc, int *tail_ok)
{
    int vis[ROWS][COLS];
    int qr[ROWS * COLS];
    int qc[ROWS * COLS];
    int front;
    int back;
    int cnt;
    int d;

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

            nr = r + drow[d];
            nc = c + dcol[d];
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

        nr = tailr + drow[d];
        nc = tailc + dcol[d];
        if (nr >= 0 && nr < ROWS &&
            nc >= 0 && nc < COLS &&
            vis[nr][nc]) {
            *tail_ok = 1;
        }
    }
    return cnt;
}

/* 低 N 时更强调短期安全与追尾保活。 */
static int choose_direction_low(void)
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

    threshold = (nval <= 1) ? 3 : 5;
    safe_food_dir = -1;
    fdir = -1;

    /* 先尝试找一条吃到食物且吃后仍安全的路径。 */
    plen = astar_state(srow, scol, slen, stepcnt, curdir,
                       foodr, foodc, pathr, pathc, &fdir);
    if (plen > 0 && fdir >= 0) {
        if (sim_follow_global(pathr, pathc, plen)) {
            extra = (plen > 15) ? plen / 3 : 0;
            if (sim_safe_like(threshold + extra)) {
                safe_food_dir = fdir;
            }
        }
    }

    /* 找不到稳吃路径时，追尾通常是更安全的兜底。 */
    tail_chase_dir = -1;
    if (nval > 1) {
        tdir = -1;
        astar_state(srow, scol, slen, stepcnt, curdir,
                    srow[slen - 1], scol[slen - 1],
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

        nr = srow[0] + drow[d];
        nc = scol[0] + dcol[d];
        if (d == (curdir ^ 2)) {
            continue;
        }
        if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
            continue;
        }
        if (wallmap[nr][nc]) {
            continue;
        }

        eating = (nr == foodr && nc == foodc) ? 1 : 0;
        growing = grow_at(eating, stepcnt + 1);
        if (bodyid[nr][nc] >= 0) {
            int istail;

            istail = (nr == srow[slen - 1] && nc == scol[slen - 1]);
            if (!(istail && !growing)) {
                continue;
            }
        }

        /* 按当前一步落点建立阻塞图，再估计后续活动空间。 */
        memcpy(blk, wallmap, sizeof(blk));
        for (i = 0; i < slen - 1; i++) {
            blk[srow[i]][scol[i]] = 1;
        }
        if (growing) {
            blk[srow[slen - 1]][scol[slen - 1]] = 1;
        }
        blk[nr][nc] = 0;

        if (growing) {
            tailr = srow[slen - 1];
            tailc = scol[slen - 1];
        } else {
            tailr = srow[slen - 2];
            tailc = scol[slen - 2];
        }

        ff = flood_eval(nr, nc, blk, tailr, tailc, &tok);
        fdist = (foodr >= 0) ? mhdist(nr, nc, foodr, foodc) : 40;

        val = ff;
        if (tok) {
            val += 5000;
        }
        if (d == safe_food_dir) {
            val += 10000;
        }
        if (eating && tok) {
            val += 8000;
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
        val += (40 - fdist) * 10;

        if (val > bestval) {
            bestval = val;
            bestd = d;
        }
    }

    /* 所有启发式都失败时，只保留合法方向作为兜底。 */
    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            int eating;
            int growing;
            int dummy_len;
            int dummy_sr[MAXSN];
            int dummy_sc[MAXSN];

            if (sim_one_step(d, dummy_sr, dummy_sc,
                             &dummy_len, &eating, &growing)) {
                bestd = d;
                break;
            }
        }
    }
    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            if (d != (curdir ^ 2)) {
                bestd = d;
                break;
            }
        }
    }
    return bestd;
}

/* 判断吃到某条路径终点后，后续局面是否依然可控。 */
static int safe_after_path(const int in_sr[], const int in_sc[],
                           int in_len, int step0,
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
    if (!sim_follow_state(in_sr, in_sc, in_len, step0, food_tr, food_tc,
                          pathr, pathc, plen,
                          tmp_sr, tmp_sc, &tmp_slen, &lastdir)) {
        return 0;
    }
    if (lastdir < 0) {
        return 0;
    }

    end_step = step0 + plen;
    space = state_space(tmp_sr, tmp_sc, tmp_slen);
    opts = state_options(tmp_sr, tmp_sc, tmp_slen, lastdir, end_step);

    if (nval <= 1) {
        need = 6 + plen / 2;
        if (tmp_slen > 200) {
            need += 2;
        }
        return space >= need && opts > 0;
    }

    /* 高 N 时要求吃完仍能通过尾部回旋，避免把自己封死。 */
    dummy = -1;
    tail_len = astar_state(tmp_sr, tmp_sc, tmp_slen, end_step, lastdir,
                           tmp_sr[tmp_slen - 1], tmp_sc[tmp_slen - 1],
                           backr, backc, &dummy);
    if (tail_len <= 0) {
        return 0;
    }

    if (nval <= 2) {
        need = 6;
    } else if (nval <= 8) {
        need = 5;
    } else {
        need = 4;
    }
    if (plen > 12) {
        need += plen / 6;
    }
    if (tmp_slen > 220) {
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

/* 仅模拟一步，作为所有启发式评分的共同入口。 */
static int sim_one_step(int d, int out_sr[], int out_sc[], int *out_len,
                        int *eating_out, int *growing_out)
{
    int nr;
    int nc;
    int eating;
    int growing;
    int i;
    int len;

    if (d == (curdir ^ 2)) {
        return 0;
    }

    nr = srow[0] + drow[d];
    nc = scol[0] + dcol[d];
    if (nr < 0 || nr >= ROWS || nc < 0 || nc >= COLS) {
        return 0;
    }
    if (wallmap[nr][nc]) {
        return 0;
    }

    eating = (nr == foodr && nc == foodc) ? 1 : 0;
    growing = grow_at(eating, stepcnt + 1);
    if (bodyid[nr][nc] >= 0) {
        int istail;

        istail = (nr == srow[slen - 1] && nc == scol[slen - 1]);
        if (!(istail && !growing)) {
            return 0;
        }
    }

    for (i = 0; i < slen; i++) {
        out_sr[i] = srow[i];
        out_sc[i] = scol[i];
    }

    len = slen;
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

/* 高 N 时采用更重的前瞻评分，避免长局中被自己包死。 */
static int choose_direction(void)
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

    fill_level = slen * 100 / 314;
    hunger = stepcnt - last_food_step;

    if (nval <= 4) {
        return choose_direction_low();
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

        if (!sim_one_step(d, nsr, nsc, &nlen, &eating, &growing)) {
            continue;
        }

        space = state_space(nsr, nsc, nlen);
        opts = state_options(nsr, nsc, nlen, d, stepcnt + 1);
        wall_touch = state_wall_touch(nsr, nsc, nlen);

        /* 先评估一步后是否还能和尾部接通。 */
        tail_len = -1;
        if (nval > 1) {
            dummy = -1;
            tail_len = astar_state(nsr, nsc, nlen, stepcnt + 1, d,
                                   nsr[nlen - 1], nsc[nlen - 1],
                                   pathr, pathc, &dummy);
        }

        /* 再评估从这一步开始能否安全接到食物。 */
        food_len = 0;
        food_safe = 0;
        if (!eating && foodr >= 0) {
            dummy = -1;
            food_len = astar_state(nsr, nsc, nlen, stepcnt + 1, d,
                                   foodr, foodc, pathr, pathc, &dummy);
            if (food_len > 0) {
                food_safe = safe_after_path(nsr, nsc, nlen, stepcnt + 1,
                                            foodr, foodc,
                                            pathr, pathc, food_len);
            }
        }

        fdist = (foodr >= 0) ? mhdist(nsr[0], nsc[0], foodr, foodc) : 40;
        val = 0;

        /* 评分项按“空间、尾部、食物、避免绕圈”组合。 */
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
            val += 24000 - food_len * ((fill_level > 60) ? 90 : 45);
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
        val -= visitcnt[nsr[0]][nsc[0]] * ((nval <= 16) ? 70 : 220);
        if (hunger > 40 && food_len > 0) {
            val += (hunger - 40) * (food_safe ? 120 : 25);
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

    /* 评分全部无效时，退回到任何一个合法方向。 */
    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            int eating;
            int growing;

            if (sim_one_step(d, nsr, nsc, &nlen, &eating, &growing)) {
                bestd = d;
                break;
            }
        }
    }
    if (bestd < 0) {
        for (d = 0; d < 4; d++) {
            if (d != (curdir ^ 2)) {
                bestd = d;
                break;
            }
        }
    }
    return bestd;
}

/* 在真实状态上执行一步移动，并更新地图和蛇身。 */
static int do_move(int d)
{
    int i;
    int nr;
    int nc;
    int eating;
    int growing;
    int otailr;
    int otailc;

    nr = srow[0] + drow[d];
    nc = scol[0] + dcol[d];
    eating = (nr == foodr && nc == foodc) ? 1 : 0;
    growing = grow_at(eating, stepcnt + 1);
    otailr = srow[slen - 1];
    otailc = scol[slen - 1];

    if (growing) {
        for (i = slen; i > 0; i--) {
            srow[i] = srow[i - 1];
            scol[i] = scol[i - 1];
        }
        slen++;
    } else {
        for (i = slen - 1; i > 0; i--) {
            srow[i] = srow[i - 1];
            scol[i] = scol[i - 1];
        }
        grid[otailr][otailc] = '.';
    }

    grid[srow[1]][scol[1]] = 'B';
    srow[0] = nr;
    scol[0] = nc;
    grid[nr][nc] = 'H';

    rebuild_bodyid(srow, scol, slen);
    curdir = d;
    stepcnt++;
    visitcnt[nr][nc]++;

    if (eating) {
        foodr = -1;
        foodc = -1;
        last_food_step = stepcnt;
    }
    return eating;
}

/* 按题目要求输出碰撞前一刻地图和得分。 */
static void print_final(void)
{
    int i;

    for (i = 0; i < ROWS; i++) {
        printf("%s\n", grid[i]);
    }
    printf("%d\n", score);
    fflush(stdout);
}
