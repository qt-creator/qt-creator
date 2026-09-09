# Threat patterns

Security patterns observed in this codebase, recorded so that a later reader
meets them before rediscovering them. Companion to `OBSERVATIONS.md` in intent:
each entry is something that cost real time to establish and is not obvious
from the code in front of you.

Categories: **Surface** (code that exposes attack surface) · **Convergence**
(several paths reaching one state without a unified check) · **Thread** (worth
following across sessions) · **Hardening** (a pattern that does security work
*well*, recorded so it is not simplified away).

**Citations name symbols, not line numbers.** The first version of this file
cited lines, and half of them were already wrong at the commit that added it —
two named symbols that commits in the same series had deleted. A symbol a
reader can grep for survives the next edit above it; a line number does not.

---

## [Convergence] — A bound denominated in a quantity the writer never spends

**Where:** `src/libs/solutions/terminal/` — the scrollback capacity passed to
`Scrollback`'s constructor in `TerminalSurfacePrivate`, which `Scrollback::trim`
spends in *logical lines* · the size given to `GlyphCache` in
`GlyphCache::instance`, in *entries* · `batchFlushSize` in `terminalsurface.cpp`,
in *bytes per tick* · `maxExtent` in `TerminalSurface::hyperlinkAt`, in *cells*.
Contrast `maxClipboardWriteSize`, which meters bytes against the selection
callback's accumulating buffer and works.

**Noticed during:** TARA, Qt Creator Terminal plugin, 2026-09-08.

**What:** Six numeric bounds exist in this subtree. **Four meter a quantity the
party supplying the input does not pay in.** The scrollback cap counts logical
lines while the writer chooses whether a row is a continuation — so a stream
with no line breaks produces one unbounded line and the cap never fires. The
glyph cache counts entries while the writer chooses each entry's shaping cost.
The drain batch is a floor on latency, not a ceiling on volume. The run-extent
bound meters run length while the writer controls the per-position lookup cost
underneath it — and that one carries an explanatory comment showing the author
anticipated the failure case and still picked the wrong unit.

**Why it matters:** A bound is adequate only if its unit dominates the *cost*
function, not the *input* function. This is not carelessness: the same author in
the same file got it right once, which is what makes it a specific
unit-selection error rather than a general absence of care. The pattern also
re-instantiated itself twice *inside* the proposed remediation — a byte budget
denominated in a container's `size()` records zero for a flood that leaves
capacity allocated, and an eviction unit of "whole line" is defeated by an
adversary who chooses line granularity.

**Possible direction:** Two units are needed here, not one — bytes for retention
and viewport-cells × miss-rate for per-frame work. When adding any bound, state
which quantity the supplier pays per byte and check the bound is in that unit.

**Citations:** CWE-407, CWE-405, CWE-770, CWE-1325, CAPEC-130. Cross-repo
catalogue: `PAT-002`.

---

## [Hardening] — A safety property held by code that was never written

**Where:** across the subtree. `terminalview.cpp` and `terminalsurface.cpp`
contain no `repaint()`, `processEvents` or nested event loop anywhere, which is
what keeps the paint path off the escape-sequence parser's stack ·
`SurfaceIntegration` in `surfaceintegration.h` has no clipboard *read* member at
all, which is the whole difference between clipboard overwrite and clipboard
theft · the `throw` in `CellIterator::operator-=` would terminate the process,
unreachable only because no real backward scan exists · the `qHash` in
`glyphcache.cpp` is visible in one translation unit · the
`m_vtermStateFallbacks` wiring in `TerminalSurfacePrivate` installs a handler
for one string-state family and so discards the others.

**Noticed during:** TARA, Qt Creator Terminal plugin, 2026-09-08.

**What:** A number of this subtree's real defences are held not by code but by
its absence — an unimplemented virtual, a missing handler, a function that
exists in only one translation unit. Alongside them sit properties held by
*accident*: the `Qt::ScrollBarAlwaysOn` policy set in `TerminalView`'s
constructor, chosen for appearance, is what prevents a reentrant resize from
inside the parser; a rounding choice is what keeps a margin smaller than a
cell; a triviality-looking zero check is the only containment for a third
writer of a width field.

**Why it matters:** These evaporate under ordinary maintenance, and **no test
goes red when they do**. A maintainer implementing the missing virtual removes a
defence without touching a line that looks defensive. One of them was
*falsified* during the assessment: "no handler installed, so the family is
discarded" turned out to be three-of-four, because one family is handled inside
the vendored parser before any host callback is consulted — which is why an
absence must be **tested**, not assumed.

**Status — partly acted on.** Two of the original examples were absences that
could be *made structural* rather than merely recorded, and were: an exported
`CellIterator` mode with no callers and contradictory would-be consumers was
deleted, and the OSC 52 clipboard-read path was removed so that no virtual
exists to implement. Both are better outcomes than a note. What remains on the
list is what could not be deleted.

**Possible direction:** For each remaining property, either delete the thing the
absence is protecting, convert it into an asserted invariant, or keep it here so
it is greppable. Note that the assertion route is currently blocked: the library
has no dependency that provides the project's soft-assert macro. Binding
condition for the pattern: the absence must be load-bearing against a *named*
threat, or it matches every unimplemented feature.

**Citations:** project-local; nearest catalogue homes CWE-1329-adjacent and
CWE-665. Cross-repo catalogue: `PAT-003`.

---

## [Surface] — The channel that shows the user an operand also resolves references inside it

**Where:** `TerminalView::showLinkToolTip` (the link tooltip) ·
`TerminalWidget::confirmUnsafePaste` with `asVisiblePasteText` (the paste
confirmation) · the `FileUtils::getOpenFilePath` call behind the "Load Theme..."
button in `terminalsettings.cpp`, which is where a confirmation would go.

**Noticed during:** TARA, Qt Creator Terminal plugin, 2026-09-08.

**What:** A surface that exists so the user can see an operand — a link's real
target, the text a paste would send — hands that operand to a widget without
naming a text format. The widget therefore applies Qt's automatic detection,
whose heuristic accepts a tag without inspecting its attributes, and the
document then loads whatever resource the attribute names. The markup can be
carried in the URL fragment, so the click payload stays valid while the display
is doing something else entirely.

**Why it matters:** This is a general shape rather than a local slip — it rests
on framework defaults, not on this code — and it inverts the usual reasoning: the
surface added *to inform a security decision* becomes an input to the attacker.
Any confirmation dialog inherits the same default. It recurs: the paste
confirmation was written after this entry and had to apply the same rule.

**Status — the two instances here are fixed; the unmeasured step stays
unmeasured.** `showLinkToolTip` escapes the target and wraps it in `<html>`;
`asVisiblePasteText` escapes the payload, and `CheckableMessageBox` names
`Qt::RichText` for it. Whether the widget requests the named resource *during
layout* was never measured, so the severity of the original chain is still
unestablished — the fixes were cheap enough not to need it. This entry is
deliberately *not* in the cross-repo catalogue for that reason: a catalogue
entry that launders an unmeasured step into a pattern is worse than no entry,
because the qualification does not travel.

**Direction, and it is the reusable part:** Make the format explicit on every
surface that displays an operand for consent, rather than leaving it to be
inferred from the operand. Which direction that is depends on the widget: one
that takes a text format can be set to plain text, but `QToolTip` takes none, so
there the explicit form is to escape the operand *and* wrap it in `<html>`.
Escaping on its own is not enough and reads as if it were — it strips the `<`
that `Qt::mightBeRichText` looks for, so an escaped operand is usually shown as
plain text with its escapes intact. Where the operand is being shown *because*
it holds control characters, escaping is also not sufficient for a second
reason: they have no glyph, so they have to be rendered visibly (caret notation)
or the reader is consenting to something still invisible.

**Citations:** CWE-838, CWE-451, CWE-1007, CAPEC-148, CAPEC-632, ATT&CK T1187.

---

## [Thread] — A performance defect that is simultaneously the safety property

**Where:** `Scrollback::Line::rowSpan` (a span walked from index zero on every
call, which is the only thing bounding four unchecked indexings) ·
`TerminalSurfacePrivate::setHyperlink` (an append-only `m_uris` table whose
unbounded growth *is* the identity guarantee for the ids stored in cells) ·
`UnixDeviceFileAccess::createTempPath` in `devicefileaccess.cpp` (the
`m_hasMkTemp` return value checked wrongly, which is what makes a weaker
fallback unreachable) · the `Qt::ScrollBarAlwaysOn` policy in `TerminalView`'s
constructor (which prevents reentrancy).

**Noticed during:** TARA, Qt Creator Terminal plugin, 2026-09-08.

**What:** In several places the obvious optimisation or the obvious correctness
fix is the vulnerability. Replacing the repeated span walk with a stored offset
table removes the derivation that currently keeps four unchecked reads in
bounds. Capping the id table creates aliasing, and because a garbage id today
almost always fails a range check while a capped table makes it pass, it would
convert a probabilistic non-link into reliable link-target confusion. Fixing the
mis-checked return value opens a race that the bug currently closes.

**Why it matters:** Each of these reads as a clean review comment. **Ten such
inversions were recorded in one assessment**, which suggests looking for the
property a defect is accidentally providing *before* removing the defect.

**Status:** single instance per site; recorded as a thread to follow rather than
an established pattern, and deliberately not in the cross-repo catalogue yet.

**Citations:** project-local; CWE-129 and CWE-125 for the span case.
