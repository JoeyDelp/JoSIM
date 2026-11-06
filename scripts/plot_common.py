"""CSV/DAT loading and output handling shared by the three plotting scripts."""
import argparse
from pathlib import Path

import numpy as np
import pandas as pd


IMAGE_FORMATS = ("png", "jpeg", "jpg", "webp", "svg", "pdf")


def dimensions(value):
    """Parse positive integer WIDTHxHEIGHT dimensions for argparse."""
    try:
        width, height = (int(part) for part in value.lower().split("x"))
        if width <= 0 or height <= 0:
            raise ValueError
        return width, height
    except ValueError as exc:
        raise argparse.ArgumentTypeError("dimensions must be positive integers, e.g. 1200x800") from exc


def read_trace(path):
    suffix = Path(path).suffix.lower()
    if suffix not in (".csv", ".dat"):
        raise ValueError("input must be a .csv or whitespace-separated .dat file")
    frame = pd.read_csv(path, sep="," if suffix == ".csv" else r"\s+")
    if frame.empty or len(frame.columns) < 2:
        raise ValueError("input must contain time samples and at least one trace")
    if not np.isfinite(frame.to_numpy(dtype=float)).all():
        raise ValueError("input contains non-finite samples")
    return frame


def protect_inputs(output, inputs):
    destination = Path(output)
    for source in map(Path, inputs):
        if (destination.resolve() == source.resolve()
                or (destination.exists() and destination.samefile(source))):
            raise ValueError("output must not overwrite an input file")


def validate_output(parser, args, inputs):
    if args.output and args.html:
        parser.error("use either --output or --html")
    try:
        if args.output or args.html:
            protect_inputs(args.output or args.html, inputs)
        if args.output and Path(args.output).suffix.lower().lstrip(".") not in IMAGE_FORMATS:
            raise ValueError("supported image formats: " + ", ".join(IMAGE_FORMATS)
                             + "; EPS is no longer supported by Kaleido")
    except (OSError, ValueError) as exc:
        parser.error(str(exc))


def validate_subset(frame, subset):
    if subset is not None:
        if not subset or any(name not in frame.columns[1:] for name in subset):
            raise ValueError("subset must name existing trace columns")


def write_plot(fig, args, config):
    if args.html:
        fig.write_html(args.html, config=config)
    elif args.output:
        # Kaleido 1 requires an external Chromium browser. Keep its sandbox on;
        # BROWSER_PATH may select a compatible installed browser explicitly.
        import kaleido
        width, height = args.dimensions or (700, 500)
        image = kaleido.calc_fig_sync(
            fig, opts={"format": Path(args.output).suffix.lower().lstrip("."),
                       "width": width, "height": height},
            kopts={"enable_sandbox": True, "timeout": 30})
        Path(args.output).write_bytes(image)
    else:
        fig.show(config=config)
