# Solutions project

The Solutions project is designed to contain a collection of
object libraries, independent on any Creator's specific code,
ready to be a part of Qt. Kind of a staging area for possible
inclusions into Qt.

## Motivation and benefits

- Such a separation will ensure no future back dependencies to the Creator
  specific code are introduced by mistake during maintenance.
- Easy to compile outside of Creator code.
- General hub of ideas to be considered by foundation team to be integrated
  into Qt.
- The more stuff of a general purpose goes into Qt, the less maintenance work
  for Creator.

## Conformity of solutions

Each solution:
- Is a separate object lib.
- Is placed in a separate subdirectory.
- Is enclosed within a namespace (namespace name = solution name).
- Should compile independently, i.e. there are no cross-includes
  between solutions.

## Dependencies of solution libraries

**Do not add dependencies to non-Qt libraries.**
The only allowed dependencies are to Qt libraries.
Especially, don't add dependencies to any Creator's library
nor to any 3rd party library.

If you can't avoid a dependency to the other Creator's library
in your solution, place it somewhere else (e.g. inside Utils library).

The Utils lib depends on the solution libraries.

## Predictions on possible integration into Qt

The solutions in this project may have a bigger / faster chance to be
integrated into Qt when they:
- Conform to Qt API style.
- Integrate easily with existing classes / types in Qt
  (instead of providing own structures / data types).
- Have full docs.
- Have auto tests.
- Have at least one example (however, autotests often play this role, too).

## Documentation

The common Solutions doc (doc/qtcreatordev/src/solutions-index.qdoc)
lists all the solutions added to the solutions-modules group.
In order to gather all the available solutions in this common page,
define the module and refer to it like:

/*!
    \module TaskingSolution
    \title The Tasking Solution
    \ingroup solutions-modules
    \brief (... add a brief description here...)
*/

The \ingroup will put the item above to the common Solutions page.
Don't add more \ingroup references from class docs, add \inmodule instead.



