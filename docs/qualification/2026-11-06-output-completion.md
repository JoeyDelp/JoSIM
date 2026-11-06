# Output completion qualification — 6 November 2026

This study resolves F21 and extends D11 beyond failures to open a file. An output
stream can open successfully and then fail during writing or its final flush.
JoSIM previously tested only `is_open()`, returning success after such failures.

## Reproduction and repair

The Linux probe created task-owned `.csv`, `.dat` and `.raw` symlinks to
`/dev/full`. This device opens successfully and rejects writes with `ENOSPC`;
no filesystem was filled. CLI CSV/DAT/RAW, a `.FILE` output and a model `.iv`
all returned **0**. Redirecting actual standard output to the same device also
returned **0**. The source decks, binary hash, commands and diagnostics are
preserved in the red receipts.

The owning writers now explicitly close file streams and check their final
state. Standard-output formatting flushes and checks `std::cout`. The existing
error owner throws a recoverable `std::runtime_error` with
`Failed to write or close output: <destination>`; the CLI catches it and returns
its established error status (255 on this Linux host). All six probes now fail
with the appropriate path or `standard output` diagnostic.

`OutputErrors::WRITE_FAILED` is appended, retaining both existing enum values.
No existing public signature, class layout, file format or numerical equation
changes. Successful output retains its existing precision and contents. Failed
output can leave a partial file; the process failure prevents it from being
reported as a successful result. The repair does not promise atomic file writes.

## Regression checks

- `regression::cli` exercises the six actual rejecting-destination cases on
  Linux, requires nonzero exit and checks the exact destination diagnostic.
  It also retains missing-directory, malformed-input and input-alias protection.
- `regression::library` uses a portable C++ stream buffer that accepts all
  character writes and fails only at `sync()`. It requires a recoverable flush
  exception, restores the original stream, then runs a valid 1 V simulation.
  Eight same-process cycles protect error recovery.
- The portable flush regression runs on every platform. The `/dev/full`
  subcases are Linux-specific; file-write fault injection on Windows/macOS is
  not inferred from the portable test. CTest has no skipped tests.

All four local configurations—GCC 16.2.1 and Clang 23.1.4, Debug and Release—
compile and pass **34/34** CTest groups. Full Clang Debug AddressSanitizer,
UndefinedBehaviorSanitizer and leak detection also pass **34/34 in 211.84 s**.
The suite retains its independent numerical assertions; this formatting change
does not relax them. Native-platform results must be followed for the pushed
revision separately.

## Evidence and reproduction

```bash
ctest --test-dir build/dev -R 'regression::(cli|library)$' --output-on-failure
```

The [development guide](../DEVELOPMENT.md) supplies full-build and sanitizer
commands. Local evidence is retained under:

- `build/audit-resolution/output-write/`: red/green decks and logs, all-four
  build/test campaign log, sanitizer build/log/JUnit result.
- `artifacts/audit-resolution/output-write/`: compact red/green receipts,
  per-configuration source/commands/JUnit results, sanitizer JUnit and exact
  repaired source hashes.

Each local build record identifies parent commit `096461b` plus the repair's
working-diff hash. The compact source manifest identifies the six implementation
and regression files independently of later documentation edits. Final integrated
qualification remains required by the audit-resolution objective.
