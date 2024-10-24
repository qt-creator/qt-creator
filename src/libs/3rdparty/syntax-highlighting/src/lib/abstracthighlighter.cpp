/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "abstracthighlighter.h"
#include "abstracthighlighter_p.h"
#include "context_p.h"
#include "definition_p.h"
#include "foldingregion.h"
#include "format.h"
#include "ksyntaxhighlighting_logging.h"
#include "repository.h"
#include "repository_p.h"
#include "rule_p.h"
#include "state.h"
#include "state_p.h"
#include "theme.h"

using namespace KSyntaxHighlighting;

AbstractHighlighterPrivate::AbstractHighlighterPrivate()
{
}

AbstractHighlighterPrivate::~AbstractHighlighterPrivate()
{
}

void AbstractHighlighterPrivate::ensureDefinitionLoaded()
{
    auto defData = DefinitionData::get(m_definition);
    if (Q_UNLIKELY(!m_definition.isValid())) {
        if (defData->repo && !defData->name.isEmpty()) {
            qCDebug(Log) << "Definition became invalid, trying re-lookup.";
            m_definition = defData->repo->definitionForName(defData->name);
            defData = DefinitionData::get(m_definition);
        }

        if (Q_UNLIKELY(!defData->repo && !defData->fileName.isEmpty())) {
            qCCritical(Log) << "Repository got deleted while a highlighter is still active!";
        }
    }

    if (m_definition.isValid()) {
        defData->load();
    }
}

AbstractHighlighter::AbstractHighlighter()
    : d_ptr(new AbstractHighlighterPrivate)
{
}

AbstractHighlighter::AbstractHighlighter(AbstractHighlighterPrivate *dd)
    : d_ptr(dd)
{
}

AbstractHighlighter::~AbstractHighlighter()
{
    delete d_ptr;
}

Definition AbstractHighlighter::definition() const
{
    return d_ptr->m_definition;
}

void AbstractHighlighter::setDefinition(const Definition &def)
{
    Q_D(AbstractHighlighter);
    d->m_definition = def;
}

Theme AbstractHighlighter::theme() const
{
    Q_D(const AbstractHighlighter);
    return d->m_theme;
}

void AbstractHighlighter::setTheme(const Theme &theme)
{
    Q_D(AbstractHighlighter);
    d->m_theme = theme;
}

/**
 * Returns the index of the first non-space character. If the line is empty,
 * or only contains white spaces, text.size() is returned.
 */
static inline int firstNonSpaceChar(QStringView text)
{
    for (int i = 0; i < text.length(); ++i) {
        if (!text[i].isSpace()) {
            return i;
        }
    }
    return text.size();
}

State AbstractHighlighter::highlightLine(QStringView text, const State &state)
{
    Q_D(AbstractHighlighter);

    // verify definition, deal with no highlighting being enabled
    d->ensureDefinitionLoaded();
    const auto defData = DefinitionData::get(d->m_definition);
    if (!d->m_definition.isValid() || !defData->isLoaded()) {
        applyFormat(0, text.size(), Format());
        return State();
    }

    // limit the cache for unification to some reasonable size
    // we use here at the moment 64k elements to not hog too much memory
    // and to make the clearing no big stall
    if (defData->unify.size() > 64 * 1024)
        defData->unify.clear();

    // verify/initialize state
    auto newState = state;
    auto stateData = StateData::get(newState);
    bool isSharedData = true;
    if (Q_UNLIKELY(stateData && stateData->m_defId != defData->id)) {
        qCDebug(Log) << "Got invalid state, resetting.";
        stateData = nullptr;
    }
    if (Q_UNLIKELY(!stateData)) {
        stateData = StateData::reset(newState);
        stateData->push(defData->initialContext(), QStringList());
        stateData->m_defId = defData->id;
        isSharedData = false;
    }

    // process empty lines
    if (Q_UNLIKELY(text.isEmpty())) {
        /**
         * handle line empty context switches
         * guard against endless loops
         * see https://phabricator.kde.org/D18509
         */
        int endlessLoopingCounter = 0;
        while (!stateData->topContext()->lineEmptyContext().isStay()) {
            /**
             * line empty context switches
             */
            if (!d->switchContext(stateData, stateData->topContext()->lineEmptyContext(), QStringList(), newState, isSharedData)) {
                /**
                 * end when trying to #pop the main context
                 */
                break;
            }

            if (stateData->topContext()->stopEmptyLineContextSwitchLoop()) {
                break;
            }

            // guard against endless loops
            ++endlessLoopingCounter;
            if (endlessLoopingCounter > 1024) {
                qCDebug(Log) << "Endless switch context transitions for line empty context, aborting highlighting of line.";
                break;
            }
        }
        auto context = stateData->topContext();
        applyFormat(0, 0, context->attributeFormat());
        return *defData->unify.insert(newState);
    }

    auto &dynamicRegexpCache = RepositoryPrivate::get(defData->repo)->m_dynamicRegexpCache;

    int offset = 0;
    int beginOffset = 0;
    bool lineContinuation = false;

    /**
     * for expensive rules like regexes we do:
     *   - match them for the complete line, as this is faster than re-trying them at all positions
     *   - store the result of the first position that matches (or -1 for no match in the full line) in the skipOffsets hash for re-use
     *   - have capturesForLastDynamicSkipOffset as guard for dynamic regexes to invalidate the cache if they might have changed
     */
    QVarLengthArray<QPair<const Rule *, int>, 8> skipOffsets;
    QStringList capturesForLastDynamicSkipOffset;

    auto getSkipOffsetValue = [&skipOffsets](const Rule *r) -> int {
        auto i = std::find_if(skipOffsets.begin(), skipOffsets.end(), [r](const auto &v) {
            return v.first == r;
        });
        if (i == skipOffsets.end())
            return 0;
        return i->second;
    };

    auto insertSkipOffset = [&skipOffsets](const Rule *r, int i) {
        auto it = std::find_if(skipOffsets.begin(), skipOffsets.end(), [r](const auto &v) {
            return v.first == r;
        });
        if (it == skipOffsets.end()) {
            skipOffsets.push_back({r, i});
        } else {
            it->second = i;
        }
    };

    /**
     * current active format
     * stored as pointer to avoid deconstruction/constructions inside the internal loop
     * the pointers are stable, the formats are either in the contexts or rules
     */
    auto currentFormat = &stateData->topContext()->attributeFormat();

    /**
     * cached first non-space character, needs to be computed if < 0
     */
    int firstNonSpace = -1;
    int lastOffset = offset;
    int endlessLoopingCounter = 0;
    do {
        /**
         * avoid that we loop endless for some broken hl definitions
         */
        if (lastOffset == offset) {
            ++endlessLoopingCounter;
            if (endlessLoopingCounter > 1024) {
                qCDebug(Log) << "Endless state transitions, aborting highlighting of line.";
                break;
            }
        } else {
            // ensure we made progress, clear the endlessLoopingCounter
            Q_ASSERT(offset > lastOffset);
            lastOffset = offset;
            endlessLoopingCounter = 0;
        }

        /**
         * try to match all rules in the context in order of declaration in XML
         */
        bool isLookAhead = false;
        int newOffset = 0;
        const Format *newFormat = nullptr;
        for (const auto &ruleShared : stateData->topContext()->rules()) {
            auto rule = ruleShared.get();
            /**
             * filter out rules that require a specific column
             */
            if ((rule->requiredColumn() >= 0) && (rule->requiredColumn() != offset)) {
                continue;
            }

            /**
             * filter out rules that only match for leading whitespace
             */
            if (rule->firstNonSpace()) {
                /**
                 * compute the first non-space lazy
                 * avoids computing it for contexts without any such rules
                 */
                if (firstNonSpace < 0) {
                    firstNonSpace = firstNonSpaceChar(text);
                }

                /**
                 * can we skip?
                 */
                if (offset > firstNonSpace) {
                    continue;
                }
            }

            int currentSkipOffset = 0;
            if (Q_UNLIKELY(rule->hasSkipOffset())) {
                /**
                 * shall we skip application of this rule? two cases:
                 *   - rule can't match at all => currentSkipOffset < 0
                 *   - rule will only match for some higher offset => currentSkipOffset > offset
                 *
                 * we need to invalidate this if we are dynamic and have different captures then last time
                 */
                if (rule->isDynamic() && (capturesForLastDynamicSkipOffset != stateData->topCaptures())) {
                    skipOffsets.clear();
                } else {
                    currentSkipOffset = getSkipOffsetValue(rule);
                    if (currentSkipOffset < 0 || currentSkipOffset > offset) {
                        continue;
                    }
                }
            }

            auto newResult = rule->doMatch(text, offset, stateData->topCaptures(), dynamicRegexpCache);
            newOffset = newResult.offset();

            /**
             * update skip offset if new one rules out any later match or is larger than current one
             */
            if (newResult.skipOffset() < 0 || newResult.skipOffset() > currentSkipOffset) {
                insertSkipOffset(rule, newResult.skipOffset());

                // remember new captures, if dynamic to enforce proper reset above on change!
                if (rule->isDynamic()) {
                    capturesForLastDynamicSkipOffset = stateData->topCaptures();
                }
            }

            if (newOffset <= offset) {
                continue;
            }

            /**
             * apply folding.
             * special cases:
             *   - rule with endRegion + beginRegion: in endRegion, the length is 0
             *   - rule with lookAhead: length is 0
             */
            if (rule->endRegion().isValid() && rule->beginRegion().isValid()) {
                applyFolding(offset, 0, rule->endRegion());
            } else if (rule->endRegion().isValid()) {
                applyFolding(offset, rule->isLookAhead() ? 0 : newOffset - offset, rule->endRegion());
            }
            if (rule->beginRegion().isValid()) {
                applyFolding(offset, rule->isLookAhead() ? 0 : newOffset - offset, rule->beginRegion());
            }

            if (rule->isLookAhead()) {
                Q_ASSERT(!rule->context().isStay());
                d->switchContext(stateData, rule->context(), std::move(newResult.captures()), newState, isSharedData);
                isLookAhead = true;
                break;
            }

            d->switchContext(stateData, rule->context(), std::move(newResult.captures()), newState, isSharedData);
            newFormat = rule->attributeFormat().isValid() ? &rule->attributeFormat() : &stateData->topContext()->attributeFormat();
            if (newOffset == text.size() && rule->isLineContinue()) {
                lineContinuation = true;
            }
            break;
        }
        if (isLookAhead) {
            continue;
        }

        if (newOffset <= offset) { // no matching rule
            if (stateData->topContext()->fallthrough()) {
                d->switchContext(stateData, stateData->topContext()->fallthroughContext(), QStringList(), newState, isSharedData);
                continue;
            }

            newOffset = offset + 1;
            newFormat = &stateData->topContext()->attributeFormat();
        }

        /**
         * if we arrive here, some new format has to be set!
         */
        Q_ASSERT(newFormat);

        /**
         * on format change, apply the last one and switch to new one
         */
        if (newFormat != currentFormat && newFormat->id() != currentFormat->id()) {
            if (offset > 0) {
                applyFormat(beginOffset, offset - beginOffset, *currentFormat);
            }
            beginOffset = offset;
            currentFormat = newFormat;
        }

        /**
         * we must have made progress if we arrive here!
         */
        Q_ASSERT(newOffset > offset);
        offset = newOffset;

    } while (offset < text.size());

    /**
     * apply format for remaining text, if any
     */
    if (beginOffset < offset) {
        applyFormat(beginOffset, text.size() - beginOffset, *currentFormat);
    }

    /**
     * handle line end context switches
     * guard against endless loops
     * see https://phabricator.kde.org/D18509
     */
    {
        int endlessLoopingCounter = 0;
        while (!stateData->topContext()->lineEndContext().isStay() && !lineContinuation) {
            if (!d->switchContext(stateData, stateData->topContext()->lineEndContext(), QStringList(), newState, isSharedData)) {
                break;
            }

            // guard against endless loops
            ++endlessLoopingCounter;
            if (endlessLoopingCounter > 1024) {
                qCDebug(Log) << "Endless switch context transitions for line end context, aborting highlighting of line.";
                break;
            }
        }
    }

    return *defData->unify.insert(newState);
}

bool AbstractHighlighterPrivate::switchContext(StateData *&data, const ContextSwitch &contextSwitch, QStringList &&captures, State &state, bool &isSharedData)
{
    const auto popCount = contextSwitch.popCount();
    const auto context = contextSwitch.context();
    if (popCount <= 0 && !context) {
        return true;
    }

    // a modified state must be detached before modification
    if (isSharedData) {
        data = StateData::detach(state);
        isSharedData = false;
    }

    // kill as many items as requested from the stack, will always keep the initial context alive!
    const bool initialContextSurvived = data->pop(popCount);

    // if we have a new context to add, push it
    // then we always "succeed"
    if (context) {
        data->push(context, std::move(captures));
        return true;
    }

    // else we abort, if we did try to pop the initial context
    return initialContextSurvived;
}

void AbstractHighlighter::applyFolding(int offset, int length, FoldingRegion region)
{
    Q_UNUSED(offset);
    Q_UNUSED(length);
    Q_UNUSED(region);
}
