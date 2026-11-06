# Independent numerical references

`buffer-101-*.cir` preserves the active reverse circuit from issue #101 and a
controlled forward comparison with the same drive and exchanged ports.
`buffer-101-*.csv` contains independent continuous Kirchhoff/RCSJ solutions,
not JoSIM output. Column names specify units. The manifest records generator
and fixture hashes, reference dependencies, pulse metrics and acceptance limits.
Hashes normalize CRLF to LF so Windows text checkout preserves provenance.

The [buffer study](../../docs/qualification/2026-11-06-buffer.md) derives the
equations, verifies the subgap assumption, checks two integration methods and
tolerances, and explains comparison budgets and startup scope.

Native `regression::buffer` needs only Python's standard library. To regenerate
the references, use the optional pinned science environment from
[DEVELOPMENT.md](../../docs/DEVELOPMENT.md), then run from the repository root:

```bash
build/science/bin/python -B tools/qualify_buffer.py \
  --output artifacts/buffer-101 --write-references
ctest --test-dir build/dev -R regression::buffer --output-on-failure --no-tests=error
```

Review generator, independent convergence, manifest and fixture changes together.
Changing tolerances to accept a failing JoSIM result is not reference validation.
Add `--binary build/dev/josim-cli --plot` for full-waveform comparison and
headless PNG/PDF plots. A reference-only run explicitly records that no JoSIM
binary was qualified; failed/interrupted runs retain an incomplete status.
