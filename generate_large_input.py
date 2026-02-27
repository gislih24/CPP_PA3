#!/usr/bin/env python3
"""
Generate a random valid arithmetic expression file for stress testing.

Default output length is 1,000,000 characters.
Expression format is a valid infix chain like:
    12+7*305-4/2+...

Notes:
- Uses only binary operators: +, -, *, /
- Avoids division-by-zero literals on the right side of '/'
- Produces exactly the requested character length
"""

from __future__ import annotations

import argparse
import random
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent


def random_number(rng: random.Random, max_len: int, non_zero: bool) -> str:
    """Create a random integer token up to max_len digits."""
    length = rng.randint(1, max_len)
    if length == 1:
        if non_zero:
            return str(rng.randint(1, 9))
        return str(rng.randint(0, 9))

    first = str(rng.randint(1, 9)) if non_zero else str(rng.randint(0, 9))
    rest = "".join(str(rng.randint(0, 9)) for _ in range(length - 1))
    return first + rest


def build_expression(target_len: int, seed: int) -> str:
    """Build a valid infix expression with exact target length."""
    if target_len < 1:
        raise ValueError("target length must be >= 1")

    rng = random.Random(seed)
    ops = ["+", "-", "*", "/"]

    # Start with one number so expression is immediately valid.
    expr = [str(rng.randint(1, 9))]
    current_len = len(expr[0])

    while current_len < target_len:
        remaining = target_len - current_len

        # Need at least operator + one digit to extend a valid expression.
        if remaining < 2:
            break

        op = rng.choice(ops)

        # If exactly 2 chars remain, we must add: <op><1-digit-number>
        if remaining == 2:
            number = str(rng.randint(1, 9) if op == "/" else rng.randint(0, 9))
            expr.append(op)
            expr.append(number)
            current_len += 2
            break

        # Leave room for at least one full future extension when possible.
        # number length can be from 1 to remaining-1 chars right now.
        max_num_len = min(8, remaining - 1)
        number = random_number(rng, max_num_len, non_zero=(op == "/"))

        # If this choice would leave exactly 1 char (invalid to finish), force 1-char number.
        future_remaining = remaining - (1 + len(number))
        if future_remaining == 1:
            number = str(rng.randint(1, 9) if op == "/" else rng.randint(0, 9))

        expr.append(op)
        expr.append(number)
        current_len += 1 + len(number)

    if current_len != target_len:
        raise RuntimeError(
            f"failed to hit exact target length: got {current_len}, expected {target_len}"
        )

    return "".join(expr)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate large random valid expression input"
    )
    parser.add_argument(
        "output",
        nargs="?",
        default=SCRIPT_DIR / "input.txt",
        type=Path,
        help="Output file path (default: script_dir/input.txt)",
    )
    parser.add_argument(
        "--length", type=int, default=1_000_000, help="Target character count"
    )
    parser.add_argument(
        "--seed", type=int, default=403, help="Random seed for reproducibility"
    )
    args = parser.parse_args()

    expression = build_expression(args.length, args.seed)
    args.output.write_text(expression, encoding="utf-8")

    print(f"Wrote {len(expression)} characters to {args.output}")


if __name__ == "__main__":
    main()
