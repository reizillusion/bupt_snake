import argparse
import math
import random
import subprocess
import sys
import time
from dataclasses import dataclass

ROWS = 20
COLS = 20
OBSTACLES = 10
DIRS = {
    "W": (-1, 0),
    "A": (0, -1),
    "S": (1, 0),
    "D": (0, 1),
}
DIR_ID = {"W": 0, "A": 1, "S": 2, "D": 3}


@dataclass
class Case:
    board: list
    nval: int


def clone_board(board):
    return [row[:] for row in board]


def board_to_lines(board):
    return ["".join(row) for row in board]


def ordered_snake(board):
    head = None
    body = set()
    for r in range(ROWS):
        for c in range(COLS):
            if board[r][c] == "H":
                head = (r, c)
            elif board[r][c] == "B":
                body.add((r, c))
    if head is None:
        raise ValueError("missing head")
    snake = [head]
    prev = None
    cur = head
    while True:
        nxt = None
        for dr, dc in DIRS.values():
            nr = cur[0] + dr
            nc = cur[1] + dc
            cell = (nr, nc)
            if nr < 0 or nr >= ROWS or nc < 0 or nc >= COLS:
                continue
            if cell == prev:
                continue
            if cell in body:
                nxt = cell
                break
        if nxt is None:
            break
        snake.append(nxt)
        body.remove(nxt)
        prev = cur
        cur = nxt
    if body:
        raise ValueError("broken snake body")
    return snake


def random_case(rng, nval):
    board = [["." for _ in range(COLS)] for _ in range(ROWS)]
    for r in range(ROWS):
        board[r][0] = "#"
        board[r][COLS - 1] = "#"
    for c in range(COLS):
        board[0][c] = "#"
        board[ROWS - 1][c] = "#"

    free = [(r, c) for r in range(1, ROWS - 1) for c in range(1, COLS - 1)]
    rng.shuffle(free)
    for r, c in free[:OBSTACLES]:
        board[r][c] = "O"

    def can_place_snake(r, c):
        return (
            2 <= r <= ROWS - 3
            and 1 <= c <= COLS - 2
            and board[r][c] == "."
            and board[r + 1][c] == "."
            and board[r + 2][c] == "."
        )

    starts = [(r, c) for r in range(1, ROWS - 2) for c in range(1, COLS - 1) if can_place_snake(r, c)]
    if not starts:
        return random_case(rng, nval)
    hr, hc = rng.choice(starts)
    board[hr][hc] = "H"
    board[hr + 1][hc] = "B"
    board[hr + 2][hc] = "B"

    empties = [(r, c) for r in range(1, ROWS - 1) for c in range(1, COLS - 1) if board[r][c] == "."]
    fr, fc = rng.choice(empties)
    board[fr][fc] = "F"
    return Case(board, nval)


class Game:
    def __init__(self, case, rng):
        self.board = clone_board(case.board)
        self.nval = case.nval
        self.rng = rng
        self.snake = ordered_snake(self.board)
        self.occ = set(self.snake)
        self.curdir = 0
        self.score = 0
        self.step = 0
        self.food = None
        for r in range(ROWS):
            for c in range(COLS):
                if self.board[r][c] == "F":
                    self.food = (r, c)
        if self.food is None:
            raise ValueError("missing food")

    def grow_now(self, eating):
        return eating or (self.step + 1) % self.nval == 0

    def random_empty(self):
        cells = []
        for r in range(1, ROWS - 1):
            for c in range(1, COLS - 1):
                if self.board[r][c] == ".":
                    cells.append((r, c))
        if not cells:
            return None
        return self.rng.choice(cells)

    def move(self, ch):
        if ch not in DIRS:
            return "bad_dir", None
        d = DIR_ID[ch]
        if d == (self.curdir ^ 2):
            return "reverse", None

        dr, dc = DIRS[ch]
        hr, hc = self.snake[0]
        nr = hr + dr
        nc = hc + dc
        eating = (nr, nc) == self.food
        growing = self.grow_now(eating)

        if self.board[nr][nc] in ("#", "O"):
            return "wall", None

        tail = self.snake[-1]
        if (nr, nc) in self.occ and not ((nr, nc) == tail and not growing):
            return "body", None

        self.board[hr][hc] = "B"
        if not growing:
            tr, tc = self.snake.pop()
            self.occ.remove((tr, tc))
            self.board[tr][tc] = "."

        self.snake.insert(0, (nr, nc))
        self.occ.add((nr, nc))
        self.board[nr][nc] = "H"
        self.curdir = d
        self.step += 1

        if eating:
            self.score += 10
            self.food = self.random_empty()
            if self.food is not None:
                fr, fc = self.food
                self.board[fr][fc] = "F"
            return "eat", self.food
        return "ok", None


def run_case(exe_path, case, seed, step_cap):
    rng = random.Random(seed)
    game = Game(case, rng)
    proc = subprocess.Popen(
        [exe_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
    )
    try:
        payload = "\n".join(board_to_lines(case.board) + [str(case.nval)]) + "\n"
        proc.stdin.write(payload)
        proc.stdin.flush()

        for _ in range(step_cap):
            move_line = proc.stdout.readline()
            score_line = proc.stdout.readline()
            if not move_line or not score_line:
                raise RuntimeError("program terminated before game end")
            move_line = move_line.strip()
            score_line = score_line.strip()
            if len(move_line) != 1 or move_line not in DIRS:
                raise RuntimeError(f"invalid move output: {move_line!r}")
            try:
                reported = int(score_line)
            except ValueError as exc:
                raise RuntimeError(f"invalid score output: {score_line!r}") from exc
            if reported != game.score:
                raise RuntimeError(f"reported score {reported} != actual {game.score}")

            result, food = game.move(move_line)
            if result in ("bad_dir", "reverse", "wall", "body"):
                proc.stdin.write("100 100\n")
                proc.stdin.flush()
                final_lines = []
                for _ in range(ROWS + 1):
                    line = proc.stdout.readline()
                    if not line:
                        break
                    final_lines.append(line.rstrip("\n"))
                proc.wait(timeout=1)
                return game.score, result, final_lines

            if result == "eat" and food is not None:
                proc.stdin.write(f"{food[0]} {food[1]}\n")
            else:
                proc.stdin.write("20 20\n")
            proc.stdin.flush()
        raise RuntimeError(f"step cap exceeded ({step_cap})")
    finally:
        if proc.poll() is None:
            proc.kill()
        try:
            proc.wait(timeout=1)
        except subprocess.TimeoutExpired:
            proc.kill()


def weighted_score(scores, nvals):
    total = 0.0
    for score, nval in zip(scores, nvals):
        total += score / (math.log2(nval) + 1.0)
    return total


def batch_run(exe_path, rounds, seed0, step_cap, time_limit_ms):
    nvals = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]
    grand = []
    for round_id in range(rounds):
        scores = []
        for idx, nval in enumerate(nvals):
            case_seed = seed0 + round_id * 1000 + idx
            map_rng = random.Random(case_seed)
            case = random_case(map_rng, nval)
            started = time.perf_counter()
            score, reason, _ = run_case(exe_path, case, case_seed ^ 0x5A5A5A5A, step_cap)
            used_ms = (time.perf_counter() - started) * 1000.0
            if time_limit_ms > 0 and used_ms > time_limit_ms:
                score = 0
                reason = "timeout"
            scores.append(score)
            print(f"round={round_id} N={nval:<3} score={score:<4} "
                  f"end={reason} ms={used_ms:.1f}")
        total = weighted_score(scores, nvals)
        grand.append(total)
        joined = " ".join(str(x) for x in scores)
        print(f"round={round_id} weighted={total:.3f} raw=[{joined}]")
    avg = sum(grand) / len(grand)
    print(f"average_weighted={avg:.3f}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", default="snake.exe")
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--seed", type=int, default=20260506)
    parser.add_argument("--step-cap", type=int, default=20000)
    parser.add_argument("--time-limit-ms", type=float, default=1000.0)
    args = parser.parse_args()
    batch_run(args.exe, args.rounds, args.seed, args.step_cap,
              args.time_limit_ms)


if __name__ == "__main__":
    sys.exit(main())
