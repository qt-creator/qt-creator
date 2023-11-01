// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// COPIED FROM qshortcutmap.cpp

#include "shortcutmap.h"

#include <utils/qtcassert.h>

#include <algorithm>

#include <QAction>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QWindow>

Q_LOGGING_CATEGORY(lcShortcutMap, "terminal.shortcutmap", QtWarningMsg)

namespace Terminal::Internal {

/* \internal
    Entry data for ShortcutMap
    Contains:
        Keysequence for entry
        Pointer to parent owning the sequence
*/
struct ShortcutEntry
{
    ShortcutEntry()
        : keyseq(0)
        , context(Qt::WindowShortcut)
        , enabled(false)
        , autorepeat(1)
        , id(0)
        , owner(nullptr)
        , contextMatcher(nullptr)
    {}

    ShortcutEntry(const QKeySequence &k)
        : keyseq(k)
        , context(Qt::WindowShortcut)
        , enabled(false)
        , autorepeat(1)
        , id(0)
        , owner(nullptr)
        , contextMatcher(nullptr)
    {}

    ShortcutEntry(QObject *o,
                  const QKeySequence &k,
                  Qt::ShortcutContext c,
                  int i,
                  bool a,
                  ShortcutMap::ContextMatcher m)
        : keyseq(k)
        , context(c)
        , enabled(true)
        , autorepeat(a)
        , id(i)
        , owner(o)
        , contextMatcher(m)
    {}

    bool correctContext() const { return contextMatcher(owner, context); }

    bool operator<(const ShortcutEntry &f) const { return keyseq < f.keyseq; }

    QKeySequence keyseq;
    Qt::ShortcutContext context;
    bool enabled : 1;
    bool autorepeat : 1;
    signed int id;
    QObject *owner;
    ShortcutMap::ContextMatcher contextMatcher;
};

/* \internal
    Private data for ShortcutMap
*/
class ShortcutMapPrivate
{
    Q_DECLARE_PUBLIC(ShortcutMap)

public:
    ShortcutMapPrivate(ShortcutMap *parent)
        : q_ptr(parent)
        , currentId(0)
        , ambigCount(0)
        , currentState(QKeySequence::NoMatch)
    {
        identicals.reserve(10);
        currentSequences.reserve(10);
    }
    ShortcutMap *q_ptr; // Private's parent

    QList<ShortcutEntry> sequences; // All sequences!

    int currentId;  // Global shortcut ID number
    int ambigCount; // Index of last enabled ambiguous dispatch
    QKeySequence::SequenceMatch currentState;
    QList<QKeySequence> currentSequences; // Sequence for the current state
    QList<QKeySequence> newEntries;
    QKeySequence prevSequence;               // Sequence for the previous identical match
    QList<const ShortcutEntry *> identicals; // Last identical matches
};

/*! \internal
    ShortcutMap constructor.
*/
ShortcutMap::ShortcutMap()
    : d_ptr(new ShortcutMapPrivate(this))
{
    resetState();
}

/*! \internal
    ShortcutMap destructor.
*/
ShortcutMap::~ShortcutMap() {}

/*! \internal
    Adds a shortcut to the global map.
    Returns the id of the newly added shortcut.
*/
int ShortcutMap::addShortcut(QObject *owner,
                             const QKeySequence &key,
                             Qt::ShortcutContext context,
                             ContextMatcher matcher)
{
    QTC_ASSERT(owner, return 0); // "ShortcutMap::addShortcut", "All shortcuts need an owner");
    QTC_ASSERT(!key.isEmpty(),
               return 0); // "ShortcutMap::addShortcut", "Cannot add keyless shortcuts to map");
    Q_D(ShortcutMap);

    ShortcutEntry newEntry(owner, key, context, --(d->currentId), true, matcher);
    const auto it = std::upper_bound(d->sequences.begin(), d->sequences.end(), newEntry);
    d->sequences.insert(it, newEntry); // Insert sorted
    qCDebug(lcShortcutMap).nospace() << "ShortcutMap::addShortcut(" << owner << ", " << key << ", "
                                     << context << ") added shortcut with ID " << d->currentId;
    return d->currentId;
}

/*! \internal
    Removes a shortcut from the global map.
    If \a owner is \nullptr, all entries in the map with the key sequence specified
    is removed. If \a key is null, all sequences for \a owner is removed from
    the map. If \a id is 0, any identical \a key sequences owned by \a owner
    are removed.
    Returns the number of sequences removed from the map.
*/

int ShortcutMap::removeShortcut(int id, QObject *owner, const QKeySequence &key)
{
    Q_D(ShortcutMap);
    int itemsRemoved = 0;
    bool allOwners = (owner == nullptr);
    bool allKeys = key.isEmpty();
    bool allIds = id == 0;

    auto debug = qScopeGuard([&]() {
        qCDebug(lcShortcutMap).nospace()
            << "ShortcutMap::removeShortcut(" << id << ", " << owner << ", " << key << ") removed "
            << itemsRemoved << " shortcuts(s)";
    });

    // Special case, remove everything
    if (allOwners && allKeys && allIds) {
        itemsRemoved = d->sequences.size();
        d->sequences.clear();
        return itemsRemoved;
    }

    int i = d->sequences.size() - 1;
    while (i >= 0) {
        const ShortcutEntry &entry = d->sequences.at(i);
        int entryId = entry.id;
        if ((allOwners || entry.owner == owner) && (allIds || entry.id == id)
            && (allKeys || entry.keyseq == key)) {
            d->sequences.removeAt(i);
            ++itemsRemoved;
        }
        if (id == entryId)
            return itemsRemoved;
        --i;
    }
    return itemsRemoved;
}

/*! \internal
    Resets the state of the statemachine to NoMatch
*/
void ShortcutMap::resetState()
{
    Q_D(ShortcutMap);
    d->currentState = QKeySequence::NoMatch;
    clearSequence(d->currentSequences);
}

/*! \internal
    Returns the current state of the statemachine
*/
QKeySequence::SequenceMatch ShortcutMap::state()
{
    Q_D(ShortcutMap);
    return d->currentState;
}

/*! \internal
    Uses nextState(QKeyEvent) to check for a grabbed shortcut.

    If so, it is dispatched using dispatchEvent().

    Returns true if a shortcut handled the event.

    \sa nextState, dispatchEvent
*/
bool ShortcutMap::tryShortcut(QKeyEvent *e)
{
    if (e->key() == Qt::Key_unknown)
        return false;

    QKeySequence::SequenceMatch previousState = state();

    switch (nextState(e)) {
    case QKeySequence::NoMatch:
        // In the case of going from a partial match to no match we handled the
        // event, since we already stated that we did for the partial match. But
        // in the normal case of directly going to no match we say we didn't.
        return previousState == QKeySequence::PartialMatch;
    case QKeySequence::PartialMatch:
        // For a partial match we don't know yet if we will handle the shortcut
        // but we need to say we did, so that we get the follow-up key-presses.
        return true;
    case QKeySequence::ExactMatch: {
        resetState();
        return dispatchEvent(e);
    }
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    Q_UNREACHABLE_RETURN(false);
#else
    return false;
#endif
}

/*! \internal
    Returns the next state of the statemachine
    If return value is SequenceMatch::ExactMatch, then a call to matches()
    will return a QObjects* list of all matching objects for the last matching
    sequence.
*/
QKeySequence::SequenceMatch ShortcutMap::nextState(QKeyEvent *e)
{
    Q_D(ShortcutMap);
    // Modifiers can NOT be shortcuts...
    if (e->key() >= Qt::Key_Shift && e->key() <= Qt::Key_ScrollLock)
        return d->currentState;

    QKeySequence::SequenceMatch result = QKeySequence::NoMatch;

    // We start fresh each time..
    d->identicals.clear();

    result = find(e);
    if (result == QKeySequence::NoMatch && (e->modifiers() & Qt::KeypadModifier)) {
        // Try to find a match without keypad modifier
        result = find(e, Qt::KeypadModifier);
    }
    if (result == QKeySequence::NoMatch && e->modifiers() & Qt::ShiftModifier) {
        // If Shift + Key_Backtab, also try Shift + Qt::Key_Tab
        if (e->key() == Qt::Key_Backtab) {
            QKeyEvent pe = QKeyEvent(e->type(), Qt::Key_Tab, e->modifiers(), e->text());
            result = find(&pe);
        }
    }

    // Does the new state require us to clean up?
    if (result == QKeySequence::NoMatch)
        clearSequence(d->currentSequences);
    d->currentState = result;

    qCDebug(lcShortcutMap).nospace() << "ShortcutMap::nextState(" << e << ") = " << result;
    return result;
}

/*! \internal
    Determines if an enabled shortcut has a matching key sequence.
*/
bool ShortcutMap::hasShortcutForKeySequence(const QKeySequence &seq) const
{
    Q_D(const ShortcutMap);
    ShortcutEntry entry(seq); // needed for searching
    const auto itEnd = d->sequences.cend();
    auto it = std::lower_bound(d->sequences.cbegin(), itEnd, entry);

    for (; it != itEnd; ++it) {
        if (matches(entry.keyseq, (*it).keyseq) == QKeySequence::ExactMatch
            && (*it).correctContext() && (*it).enabled) {
            return true;
        }
    }

    //end of the loop: we didn't find anything
    return false;
}

/*! \internal
    Returns the next state of the statemachine, based
    on the new key event \a e.
    Matches are appended to the list of identicals,
    which can be access through matches().
    \sa matches
*/
QKeySequence::SequenceMatch ShortcutMap::find(QKeyEvent *e, int ignoredModifiers)
{
    Q_D(ShortcutMap);
    if (!d->sequences.size())
        return QKeySequence::NoMatch;

    createNewSequences(e, d->newEntries, ignoredModifiers);
    qCDebug(lcShortcutMap) << "Possible shortcut key sequences:" << d->newEntries;

    // Should never happen
    if (d->newEntries == d->currentSequences) {
        QTC_ASSERT(e->key() != Qt::Key_unknown || e->text().size(), return QKeySequence::NoMatch);
        //"ShortcutMap::find",
        // "New sequence to find identical to previous");
        return QKeySequence::NoMatch;
    }

    // Looking for new identicals, scrap old
    d->identicals.clear();

    bool partialFound = false;
    bool identicalDisabledFound = false;
    QList<QKeySequence> okEntries;
    int result = QKeySequence::NoMatch;
    for (int i = d->newEntries.size() - 1; i >= 0; --i) {
        ShortcutEntry entry(d->newEntries.at(i)); // needed for searching
        qCDebug(lcShortcutMap) << "- checking entry" << entry.id << entry.keyseq;
        const auto itEnd = d->sequences.constEnd();
        auto it = std::lower_bound(d->sequences.constBegin(), itEnd, entry);

        int oneKSResult = QKeySequence::NoMatch;
        int tempRes = QKeySequence::NoMatch;
        do {
            if (it == itEnd)
                break;
            tempRes = matches(entry.keyseq, (*it).keyseq);
            oneKSResult = qMax(oneKSResult, tempRes);
            qCDebug(lcShortcutMap) << "  - matches returned" << tempRes << "for" << entry.keyseq
                                   << it->keyseq << "- correctContext()?" << it->correctContext();
            if (tempRes != QKeySequence::NoMatch && (*it).correctContext()) {
                if (tempRes == QKeySequence::ExactMatch) {
                    if ((*it).enabled)
                        d->identicals.append(&*it);
                    else
                        identicalDisabledFound = true;
                } else if (tempRes == QKeySequence::PartialMatch) {
                    // We don't need partials, if we have identicals
                    if (d->identicals.size())
                        break;
                    // We only care about enabled partials, so we don't consume
                    // key events when all partials are disabled!
                    partialFound |= (*it).enabled;
                }
            }
            ++it;
            // If we got a valid match on this run, there might still be more keys to check against,
            // so we'll loop once more. If we get NoMatch, there's guaranteed no more possible
            // matches in the shortcutmap.
        } while (tempRes != QKeySequence::NoMatch);

        // If the type of match improves (ergo, NoMatch->Partial, or Partial->Exact), clear the
        // previous list. If this match is equal or better than the last match, append to the list
        if (oneKSResult > result) {
            okEntries.clear();
            qCDebug(lcShortcutMap)
                << "Found better match (" << d->newEntries << "), clearing key sequence list";
        }
        if (oneKSResult && oneKSResult >= result) {
            okEntries << d->newEntries.at(i);
            qCDebug(lcShortcutMap) << "Added ok key sequence" << d->newEntries;
        }
    }

    if (d->identicals.size()) {
        result = QKeySequence::ExactMatch;
    } else if (partialFound) {
        result = QKeySequence::PartialMatch;
    } else if (identicalDisabledFound) {
        result = QKeySequence::ExactMatch;
    } else {
        clearSequence(d->currentSequences);
        result = QKeySequence::NoMatch;
    }
    if (result != QKeySequence::NoMatch)
        d->currentSequences = okEntries;
    qCDebug(lcShortcutMap) << "Returning shortcut match == " << result;
    return QKeySequence::SequenceMatch(result);
}

/*! \internal
    Clears \a seq to an empty QKeySequence.
    Same as doing (the slower)
    \snippet code/src_gui_kernel_shortcutmap.cpp 0
*/
void ShortcutMap::clearSequence(QList<QKeySequence> &ksl)
{
    ksl.clear();
    d_func()->newEntries.clear();
}

static QList<int> extractKeyFromEvent(QKeyEvent *e)
{
    QList<int> result;
    if (e->key() && (e->key() != Qt::Key_unknown))
        result << e->keyCombination().toCombined();
    else if (!e->text().isEmpty())
        result << int(e->text().at(0).unicode() + (int) e->modifiers());
    return result;
}

/*! \internal
    Alters \a seq to the new sequence state, based on the
    current sequence state, and the new key event \a e.
*/
void ShortcutMap::createNewSequences(QKeyEvent *e, QList<QKeySequence> &ksl, int ignoredModifiers)
{
    Q_D(ShortcutMap);

    QList<int> possibleKeys = extractKeyFromEvent(e);
    qCDebug(lcShortcutMap) << "Creating new sequences for" << e
                           << "with ignoredModifiers=" << Qt::KeyboardModifiers(ignoredModifiers);
    int pkTotal = possibleKeys.size();
    if (!pkTotal)
        return;

    int ssActual = d->currentSequences.size();
    int ssTotal = qMax(1, ssActual);
    // Resize to possible permutations of the current sequence(s).
    ksl.resize(pkTotal * ssTotal);

    int index = ssActual ? d->currentSequences.at(0).count() : 0;
    for (int pkNum = 0; pkNum < pkTotal; ++pkNum) {
        for (int ssNum = 0; ssNum < ssTotal; ++ssNum) {
            int i = (pkNum * ssTotal) + ssNum;
            QKeySequence &curKsl = ksl[i];
            if (ssActual) {
                const QKeySequence &curSeq = d->currentSequences.at(ssNum);
                curKsl = QKeySequence(curSeq[0], curSeq[1], curSeq[2], curSeq[3]);
            } else {
                curKsl = QKeySequence(QKeyCombination::fromCombined(0));
            }

            std::array<QKeyCombination, 4> cur = {curKsl[0], curKsl[1], curKsl[2], curKsl[3]};
            cur[index] = QKeyCombination::fromCombined(possibleKeys.at(pkNum) & ~ignoredModifiers);
            curKsl = QKeySequence(cur[0], cur[1], cur[2], cur[3]);
        }
    }
}

/*! \internal
    Basically the same function as QKeySequence::matches(const QKeySequence &seq) const
    only that is specially handles Key_hyphen as Key_Minus, as people mix these up all the time and
    they conceptually the same.
*/
QKeySequence::SequenceMatch ShortcutMap::matches(const QKeySequence &seq1,
                                                 const QKeySequence &seq2) const
{
    uint userN = seq1.count(), seqN = seq2.count();

    if (userN > seqN)
        return QKeySequence::NoMatch;

    // If equal in length, we have a potential ExactMatch sequence,
    // else we already know it can only be partial.
    QKeySequence::SequenceMatch match = (userN == seqN ? QKeySequence::ExactMatch
                                                       : QKeySequence::PartialMatch);

    for (uint i = 0; i < userN; ++i) {
        int userKey = seq1[i].toCombined(), sequenceKey = seq2[i].toCombined();
        if ((userKey & Qt::Key_unknown) == Qt::Key_hyphen)
            userKey = (userKey & Qt::KeyboardModifierMask) | Qt::Key_Minus;
        if ((sequenceKey & Qt::Key_unknown) == Qt::Key_hyphen)
            sequenceKey = (sequenceKey & Qt::KeyboardModifierMask) | Qt::Key_Minus;
        if (userKey != sequenceKey)
            return QKeySequence::NoMatch;
    }
    return match;
}

/*! \internal
    Returns the list of ShortcutEntry's matching the last Identical state.
*/
QList<const ShortcutEntry *> ShortcutMap::matches() const
{
    Q_D(const ShortcutMap);
    return d->identicals;
}

/*! \internal
    Dispatches QShortcutEvents to widgets who grabbed the matched key sequence.
*/
bool ShortcutMap::dispatchEvent(QKeyEvent *e)
{
    Q_D(ShortcutMap);
    if (!d->identicals.size())
        return false;

    const QKeySequence &curKey = d->identicals.at(0)->keyseq;
    if (d->prevSequence != curKey) {
        d->ambigCount = 0;
        d->prevSequence = curKey;
    }
    // Find next
    const ShortcutEntry *current = nullptr, *next = nullptr;
    int i = 0, enabledShortcuts = 0;
    QList<const ShortcutEntry *> ambiguousShortcuts;
    while (i < d->identicals.size()) {
        current = d->identicals.at(i);
        if (current->enabled || !next) {
            ++enabledShortcuts;
            if (lcShortcutMap().isDebugEnabled())
                ambiguousShortcuts.append(current);
            if (enabledShortcuts > d->ambigCount + 1)
                break;
            next = current;
        }
        ++i;
    }
    d->ambigCount = (d->identicals.size() == i ? 0 : d->ambigCount + 1);
    // Don't trigger shortcut if we're autorepeating and the shortcut is
    // grabbed with not accepting autorepeats.
    if (!next || (e->isAutoRepeat() && !next->autorepeat))
        return false;
    // Dispatch next enabled
    if (lcShortcutMap().isDebugEnabled()) {
        if (ambiguousShortcuts.size() > 1) {
            qCDebug(lcShortcutMap)
                << "The following shortcuts are about to be activated ambiguously:";
            for (const ShortcutEntry *entry : std::as_const(ambiguousShortcuts))
                qCDebug(lcShortcutMap).nospace()
                    << "- " << entry->keyseq << " (belonging to " << entry->owner << ")";
        }

        qCDebug(lcShortcutMap).nospace()
            << "ShortcutMap::dispatchEvent(): Sending QShortcutEvent(\"" << next->keyseq.toString()
            << "\", " << next->id << ", " << static_cast<bool>(enabledShortcuts > 1)
            << ") to object(" << next->owner << ')';
    }

    if (auto action = qobject_cast<QAction *>(next->owner)) {
        // We call the action here ourselves instead of relying on sending a ShortCut event,
        // as the action will try to match the shortcut id to the global shortcutmap.
        // This triggers an annoying Q_ASSERT when linking against a debug Qt. Calling trigger
        // directly circumvents this.
        action->trigger();
        return action->isEnabled();
    } else {
        QShortcutEvent se(next->keyseq, next->id, enabledShortcuts > 1);
        QCoreApplication::sendEvent(const_cast<QObject *>(next->owner), &se);
    }

    return true;
}

} // namespace Terminal::Internal
