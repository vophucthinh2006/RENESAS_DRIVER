---
type: phase
phase: 1
status: done
completed: 2026-04-22
---

# Phase 1 — Fix Header Dependencies & SYSC Macro Conflict

Back to [[INDEX]] | Next: [[PHASE_2]]

Bugs fixed: [[BUGS]]

---

## Objective

Eliminate macro redefinition of `SYSC` and make every header self-contained.

## Bugs Addressed

- BUG-01 — `SYSC` defined in both `LPM.h` and `RWP.h`
- BUG-02 — `CGC.h` uses `SYSC` without including it
- S-04 — `SYSC` double-defined (structural)

## Tasks

- [x] P1-1 Create `Driver/Include/drv_common.h` with single `SYSC` definition
- [x] P1-2 Remove `#define SYSC` from `LPM.h`; add `#include "drv_common.h"`
- [x] P1-3 Remove `#define SYSC` from `RWP.h`; add `#include "drv_common.h"`
- [x] P1-4 Add `#include "drv_common.h"` to `CGC.h`
- [x] P1-5 Build verify — zero macro redefinition warnings

## Changes Made

**New file:** `Driver/Include/drv_common.h`
- Defines `SYSC 0x4001E000UL` as single source of truth
- Include guard: `DRV_COMMON_H`

**Modified:** `Driver/Include/LPM.h`
- Removed `#define SYSC 0x4001E000UL`
- Added `#include "drv_common.h"`

**Modified:** `Driver/Include/RWP.h`
- Removed `#define SYSC 0x4001E000UL`
- Added `#include "drv_common.h"`

**Modified:** `Driver/Include/CGC.h`
- Added `#include "drv_common.h"`

## Dependency after this phase

drv_common.h is included by LPM.h, RWP.h, CGC.h

File map updated → [[FILES]]

## Status: ✅ DONE
