import argparse
import random
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Generate an obstacle course.")
    parser.add_argument("--length", type=int, default=1000)
    parser.add_argument("--seed", type=int, default=None)
    parser.add_argument("--output", type=Path, default=Path(__file__).with_name("track.txt"))
    args = parser.parse_args()

    if args.length < 1:
        parser.error("--length must be at least 1")

    rng = random.Random(args.seed)
    obstacles = [-1 if rng.random() < 0.35 else rng.randrange(3)
                 for _ in range(args.length)]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        "\n".join(str(lane) for lane in obstacles) + "\n",
        encoding="utf-8",
    )
    print(f"Generated {args.length} track sections: {args.output}")


if __name__ == "__main__":
    main()
