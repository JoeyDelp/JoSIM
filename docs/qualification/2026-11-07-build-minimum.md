# Minimum build-tool qualification — 7 November 2026

The supported workflow requires CMake/CTest **3.21 or newer**. Qualification
owners consume CTest's JUnit output; declaring 3.14 while requiring a newer
test workflow gave an inconsistent build contract. `CMakeLists.txt` now declares
3.21, matching the development and release instructions.

An isolated, task-owned PyPI installation of exactly CMake/CTest **3.21.0**
configures and compiles the current candidate with local GNU 16.2.1 and Ninja.
All **35/35** declared CTest groups pass, including an installed library moved
to a path with spaces and a newly compiled external consumer. The current F27
candidate is included. The total configure/build/test campaign takes 27.59 s;
this duration is verification provenance, not a performance benchmark.

The source remains C11/C++17. This build-tool decision changes no declared
binary runtime baseline. It qualifies the local default-library workflow;
native platform/library configurations and older OS/runtime checks have their
own gates. It does not assert every old CMake/compiler/OS combination works.

`artifacts/audit-resolution/cmake-min/` retains exact commands, source/owner
identities, the candidate patch, install metadata and test inventory/JUnit.
`qualify_minimum.py` reproduces the task-owned campaign from the repository root
with its recorded 3.21.0 environment and prepared dependency cache. No global
CMake installation was changed. An earlier attempt requested 3.21.7, which has
no PyPI distribution; that diagnostic is retained, and is not a passing check.
