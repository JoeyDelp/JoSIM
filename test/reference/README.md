# Independent numerical references

`buffer-101-*.cir` contains the reverse buffer circuit from issue #101 and a
forward comparison with the same drive and exchanged ports. The CSV files
contain independent Kirchhoff/RCSJ solutions, calculated with Radau and BDF
integration. Column names specify units. The JSON manifest records the generator,
dependencies, fixture hashes and comparison tolerances; hashes normalize CRLF
to LF for Windows checkouts.

The [buffer example](../../docs/numerics.md#buffer) explains the bipolar response
and pulse areas. The CTest `regression::buffer` comparison uses Python's standard
library. To regenerate the reference data, install the optional scientific
dependencies from [DEVELOPMENT.md](../../docs/DEVELOPMENT.md), then run:

```bash
build/science/bin/python -B tools/qualify_buffer.py \
  --output artifacts/buffer-101 --write-references
ctest --test-dir build/dev -R regression::buffer --output-on-failure --no-tests=error
```

Add `--binary build/dev/josim-cli --plot` to compare JoSIM waveforms and produce
PNG/PDF plots. Review changes to the generator, data and tolerances together.
