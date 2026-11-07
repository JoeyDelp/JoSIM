# Public download verification — preparation, 7 November 2026

The 3.0.0 candidate at `5a27110` passes all twelve native/library/utility/coverage
CI jobs (run42314611144). This preparation adds a documentation prerequisite and
postpublication verification. Actual public 3.0.0 downloads and execution remain
pending the final source gates, permitted integration, tag and publication.

`tools/verify_release.py` resolves the public tag to the expected full commit,
requires a published stable release, and downloads the real archive, checksum
and sidecar manifest. It checks API size/digest where provided, the sidecar hash,
embedded-manifest agreement, source/version/target/clean identity, exact payload
inventory and every payload size/hash. Extraction rejects escaping paths, links,
special files and duplicate names; packaged local project context is rejected.

Native execution checks the host/architecture and CLI source/version banner,
then runs the CLI regressions and independent numerical suite against the actual
downloaded binary. Empty, skipped or failed runs and incomplete/wrong-binary
numerical reports fail. Source/test-owner and executable identities are checked
and recorded. JSON and readable receipts distinguish `prepared` downloads from
`passed` native execution.

The reusable `verify-release.yml` runs five targets. Linux downloads on a modern
host and executes in the same pinned legacy userspace used for package builds,
with network disabled. Windows and both macOS architectures use native runners.
The tagged publisher invokes this workflow after upload. Manual dispatch accepts
a public tag/full commit and records the verification source separately. Asset
HTTP requests are anonymous; the optional CI read token is used only for metadata.

The publisher now uses reviewed versioned notes and disables asset overwrites.
The documentation prerequisite builds the pinned manual in strict mode and
checks every local file/fragment plus the context exclusion. Branch builds do
not publish a release or claim that postpublication tests have run.

## Preparatory checks

- Thirteen tool methods pass, including synthetic archive/manifest/API/execution
  contracts and the earlier complete-inventory/coverage guards. New negatives
  cover wrong tag/source/target/version, draft/prerelease, dirty/skipped states,
  bad hashes/sizes, missing/extra/duplicate files, path escapes, archive links,
  embedded/sidecar disagreement, local context, empty/skipped/failed execution,
  incomplete numerical data and relabelling a prepared download.
- A synthetic download checks that all asset requests omit credentials even
  when an API token is configured. Fixture executables are never run. These
  contracts do not constitute public binary qualification.
- The live metadata/tag resolver identifies preserved v2.7 at `02a34ee5`; no
  old binary download or execution is claimed by that read-only check.
- Seventeen Bash workflow blocks parse. The strict manual build passes with
  54 pages and 2,334 local links before adding this preparation record.
- Focused qualification/CLI/numerical/installed-consumer groups pass 4/4 in
  4.04 s. New workflow/native/final integrated-source checks remain required.

`artifacts/audit-resolution/public-download-preparation/` retains unit/focused
results, workflow syntax receipts, live API metadata scope, source/owner hashes
and exact commands. Actual release runs will retain their own public URLs,
downloads, native execution results and numerical reports; these are separate
from prepublication package extraction and the synthetic contracts above.

## Windows archive-name follow-up

The first 9ae6f0a Windows CI run passed 34/35 groups but failed the new archive
fixture: `ZipInfo` changed `bin\escape` to `bin/escape` before the asserted
rejection. The fixture therefore did not contain its intended raw name on
Windows. The reader also exposes the original stored name separately.

The fixture now writes the literal header name, and the verifier checks
`orig_filename` before normalization. The same unsafe bytes are rejected with
both native and Windows-normalizing readers; no assertion or path rule is
relaxed. All thirteen guard methods pass on native x870-win10/Python 3.12.10
in 0.547 s and in all five local compiler/sanitizer test configurations. Native
PowerShell parsing also accepts the workflow's 4 PowerShell blocks; it does
not execute release commands. `public-download-preparation/windows/` retains
old/new normalization witnesses, CI failure scope and native source/hash receipts.

Before this tooling-only repair, the unchanged first-party C++ source passes
four fresh local 35-group configurations, full ASan/UBSan/leaks 35/35 in
216.85 s, the reviewed 45 static-warning locations, and coverage requirements at
82.43%/89.23%/79.38% line/function/branch. Final native/integrated qualification
of the corrected verifier remains required.
