/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "highlighter.h"
#include "highlightdefinition.h"
#include "context.h"
#include "rule.h"
#include "itemdata.h"
#include "highlighterexception.h"
#include "progressdata.h"
#include "reuse.h"
#include "tabsettings.h"

#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace TextEditor;
using namespace Internal;

namespace {
    static const QLatin1String kStay("#stay");
    static const QLatin1String kPop("#pop");
    static const QLatin1Char kBackSlash('\\');
    static const QLatin1Char kHash('#');
    static const QLatin1Char kExclamationMark('!');
}

class HighlighterCodeFormatterData : public CodeFormatterData
{
public:
    HighlighterCodeFormatterData() :
        m_foldingIndentDelta(0),
        m_originalObservableState(-1),
        m_continueObservableState(-1)
    {}

    ~HighlighterCodeFormatterData() {}
    int m_foldingIndentDelta;
    int m_originalObservableState;
    QStack<QString> m_foldingRegions;
    int m_continueObservableState;
};

HighlighterCodeFormatterData *formatterData(const QTextBlock &block)
{
    HighlighterCodeFormatterData *data = 0;
    if (TextBlockUserData *userData = TextDocumentLayout::userData(block)) {
        data = static_cast<HighlighterCodeFormatterData *>(userData->codeFormatterData());
        if (!data) {
            data = new HighlighterCodeFormatterData;
            userData->setCodeFormatterData(data);
        }
    }
    return data;
}

static TextStyle styleForFormat(int format)
{
    const auto f = Highlighter::TextFormatId(format);
    switch (f) {
    case Highlighter::Normal: return C_TEXT;
    case Highlighter::Keyword: return C_KEYWORD;
    case Highlighter::DataType: return C_TYPE;
    case Highlighter::Comment: return C_COMMENT;
    case Highlighter::Decimal: return C_NUMBER;
    case Highlighter::BaseN: return C_NUMBER;
    case Highlighter::Float: return C_NUMBER;
    case Highlighter::Char: return C_STRING;
    case Highlighter::SpecialChar: return C_STRING;
    case Highlighter::String: return C_STRING;
    case Highlighter::Alert: return C_WARNING;
    case Highlighter::Information: return C_TEXT;
    case Highlighter::Warning: return C_WARNING;
    case Highlighter::Error: return C_ERROR;
    case Highlighter::Function: return C_FUNCTION;
    case Highlighter::RegionMarker: return C_TEXT;
    case Highlighter::BuiltIn: return C_PREPROCESSOR;
    case Highlighter::Extension: return C_PRIMITIVE_TYPE;
    case Highlighter::Operator: return C_OPERATOR;
    case Highlighter::Variable: return C_LOCAL;
    case Highlighter::Attribute: return C_LABEL;
    case Highlighter::Annotation: return C_TEXT;
    case Highlighter::CommentVar: return C_COMMENT;
    case Highlighter::Import: return C_PREPROCESSOR;
    case Highlighter::Others: return C_TEXT;
    case Highlighter::Identifier: return C_LOCAL;
    case Highlighter::Documentation: return C_DOXYGEN_COMMENT;
    case Highlighter::ControlFlow: return C_KEYWORD;
    case Highlighter::Preprocessor: return C_PREPROCESSOR;
    case Highlighter::VerbatimString: return C_STRING;
    case Highlighter::SpecialString: return C_STRING;
    case Highlighter::Constant: return C_KEYWORD;
    case Highlighter::TextFormatIdCount:
        QTC_CHECK(false); // should never get here
        return C_TEXT;
    }
    QTC_CHECK(false); // should never get here
    return C_TEXT;
}

Highlighter::Highlighter(QTextDocument *parent) :
    SyntaxHighlighter(parent),
    m_regionDepth(0),
    m_indentationBasedFolding(false),
    m_tabSettings(0),
    m_persistentObservableStatesCounter(PersistentsStart),
    m_dynamicContextsCounter(0),
    m_isBroken(false)
{
    setTextFormatCategories(TextFormatIdCount, styleForFormat);
}

Highlighter::~Highlighter()
{}

// Mapping from Kate format strings to format ids.
struct KateFormatMap
{
    KateFormatMap();
    QHash<QString, Highlighter::TextFormatId> m_ids;
};

KateFormatMap::KateFormatMap()
{
    m_ids.insert(QLatin1String("dsNormal"), Highlighter::Normal);
    m_ids.insert(QLatin1String("dsKeyword"), Highlighter::Keyword);
    m_ids.insert(QLatin1String("dsDataType"), Highlighter::DataType);
    m_ids.insert(QLatin1String("dsComment"), Highlighter::Comment);
    m_ids.insert(QLatin1String("dsDecVal"), Highlighter::Decimal);
    m_ids.insert(QLatin1String("dsBaseN"), Highlighter::BaseN);
    m_ids.insert(QLatin1String("dsFloat"), Highlighter::Float);
    m_ids.insert(QLatin1String("dsChar"), Highlighter::Char);
    m_ids.insert(QLatin1String("dsSpecialChar"), Highlighter::SpecialChar);
    m_ids.insert(QLatin1String("dsString"), Highlighter::String);
    m_ids.insert(QLatin1String("dsAlert"), Highlighter::Alert);
    m_ids.insert(QLatin1String("dsInformation"), Highlighter::Information);
    m_ids.insert(QLatin1String("dsWarning"), Highlighter::Warning);
    m_ids.insert(QLatin1String("dsError"), Highlighter::Error);
    m_ids.insert(QLatin1String("dsFunction"), Highlighter::Function);
    m_ids.insert(QLatin1String("dsRegionMarker"), Highlighter::RegionMarker);
    m_ids.insert(QLatin1String("dsBuiltIn"), Highlighter::BuiltIn);
    m_ids.insert(QLatin1String("dsExtension"), Highlighter::Extension);
    m_ids.insert(QLatin1String("dsOperator"), Highlighter::Operator);
    m_ids.insert(QLatin1String("dsVariable"), Highlighter::Variable);
    m_ids.insert(QLatin1String("dsAttribute"), Highlighter::Attribute);
    m_ids.insert(QLatin1String("dsAnnotation"), Highlighter::Annotation);
    m_ids.insert(QLatin1String("dsCommentVar"), Highlighter::CommentVar);
    m_ids.insert(QLatin1String("dsImport"), Highlighter::Import);
    m_ids.insert(QLatin1String("dsOthers"), Highlighter::Others);
    m_ids.insert(QLatin1String("dsIdentifier"), Highlighter::Identifier);
    m_ids.insert(QLatin1String("dsDocumentation"), Highlighter::Documentation);
    m_ids.insert(QLatin1String("dsControlFlow"), Highlighter::ControlFlow);
    m_ids.insert(QLatin1String("dsPreprocessor"), Highlighter::Preprocessor);
    m_ids.insert(QLatin1String("dsVerbatimString"), Highlighter::VerbatimString);
    m_ids.insert(QLatin1String("dsSpecialString"), Highlighter::SpecialString);
    m_ids.insert(QLatin1String("dsConstant"), Highlighter::Constant);
}

Q_GLOBAL_STATIC(KateFormatMap, kateFormatMap)

void Highlighter::setDefaultContext(const QSharedPointer<Context> &defaultContext)
{
    m_defaultContext = defaultContext;
    m_persistentObservableStates.insert(m_defaultContext->name(), Default);
    m_indentationBasedFolding = defaultContext->definition()->isIndentationBasedFolding();
}

void Highlighter::setTabSettings(const TabSettings &ts)
{
    m_tabSettings = &ts;
}

static bool isOpeningParenthesis(QChar c)
{
    return c == QLatin1Char('{') || c == QLatin1Char('[') || c == QLatin1Char('(');
}

static bool isClosingParenthesis(QChar c)
{
    return c == QLatin1Char('}') || c == QLatin1Char(']') || c == QLatin1Char(')');
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!m_defaultContext.isNull() && !m_isBroken) {
        try {
            setupDataForBlock(text);

            handleContextChange(m_currentContext->lineBeginContext(),
                                m_currentContext->definition());

            ProgressData *progress = new ProgressData;
            const int length = text.length();
            while (progress->offset() < length)
                iterateThroughRules(text, length, progress, false, m_currentContext->rules());

            if (extractObservableState(currentBlockState()) != WillContinue) {
                handleContextChange(m_currentContext->lineEndContext(),
                                    m_currentContext->definition(),
                                    false);
            }
            if (length == 0) {
                handleContextChange(m_currentContext->lineEmptyContext(),
                                    m_currentContext->definition(),
                                    false);
            }
            delete progress;
            m_contexts.clear();

            if (m_indentationBasedFolding) {
                applyIndentationBasedFolding(text);
            } else {
                applyRegionBasedFolding();

                // In the case region depth has changed since the last time the state was set.
                setCurrentBlockState(computeState(extractObservableState(currentBlockState())));
            }

            Parentheses parentheses;
            for (int pos = 0; pos < length; ++pos) {
                const QChar c = text.at(pos);
                if (isOpeningParenthesis(c))
                    parentheses.push_back(Parenthesis(Parenthesis::Opened, c, pos));
                else if (isClosingParenthesis(c))
                    parentheses.push_back(Parenthesis(Parenthesis::Closed, c, pos));
            }
            TextDocumentLayout::setParentheses(currentBlock(), parentheses);

        } catch (const HighlighterException &e) {
            Core::MessageManager::write(
                        QCoreApplication::translate("GenericHighlighter",
                                                    "Generic highlighter error: %1")
                                                    .arg(e.message()),
                                                    Core::MessageManager::WithFocus);
            m_isBroken = true;
        }
    }

    formatSpaces(text);
}

void Highlighter::setupDataForBlock(const QString &text)
{
    if (extractObservableState(currentBlockState()) == WillContinue)
        analyseConsistencyOfWillContinueBlock(text);

    if (previousBlockState() == -1) {
        m_regionDepth = 0;
        setupDefault();
    } else {
        m_regionDepth = extractRegionDepth(previousBlockState());
        const int observablePreviousState = extractObservableState(previousBlockState());
        if (observablePreviousState == Default)
            setupDefault();
        else if (observablePreviousState == WillContinue)
            setupFromWillContinue();
        else if (observablePreviousState == Continued)
            setupFromContinued();
        else
            setupFromPersistent();

        formatterData(currentBlock())->m_foldingRegions =
                formatterData(currentBlock().previous())->m_foldingRegions;
    }

    assignCurrentContext();
}

void Highlighter::setupDefault()
{
    m_contexts.push_back(m_defaultContext);

    setCurrentBlockState(computeState(Default));
}

void Highlighter::setupFromWillContinue()
{
    HighlighterCodeFormatterData *previousData = formatterData(currentBlock().previous());
    pushContextSequence(previousData->m_continueObservableState);

    HighlighterCodeFormatterData *data = formatterData(currentBlock());
    data->m_originalObservableState = previousData->m_originalObservableState;

    if (currentBlockState() == -1 || extractObservableState(currentBlockState()) == Default)
        setCurrentBlockState(computeState(Continued));
}

void Highlighter::setupFromContinued()
{
    HighlighterCodeFormatterData *previousData = formatterData(currentBlock().previous());

    Q_ASSERT(previousData->m_originalObservableState != WillContinue &&
             previousData->m_originalObservableState != Continued);

    if (previousData->m_originalObservableState == Default ||
        previousData->m_originalObservableState == -1) {
        m_contexts.push_back(m_defaultContext);
    } else {
        pushContextSequence(previousData->m_originalObservableState);
    }

    setCurrentBlockState(computeState(previousData->m_originalObservableState));
}

void Highlighter::setupFromPersistent()
{
    pushContextSequence(extractObservableState(previousBlockState()));

    setCurrentBlockState(previousBlockState());
}

void Highlighter::iterateThroughRules(const QString &text,
                                      const int length,
                                      ProgressData *progress,
                                      const bool childRule,
                                      const QList<QSharedPointer<Rule> > &rules)
{
    typedef QList<QSharedPointer<Rule> >::const_iterator RuleIterator;

    bool contextChanged = false;
    bool atLeastOneMatch = false;

    RuleIterator it = rules.begin();
    RuleIterator endIt = rules.end();
    while (it != endIt && progress->offset() < length) {
        int startOffset = progress->offset();
        const QSharedPointer<Rule> &rule = *it;
        if (rule->matchSucceed(text, length, progress)) {
            atLeastOneMatch = true;

            if (!m_indentationBasedFolding) {
                if (!rule->beginRegion().isEmpty()) {
                    formatterData(currentBlock())->m_foldingRegions.push(rule->beginRegion());
                    ++m_regionDepth;
                    if (progress->isOpeningBraceMatchAtFirstNonSpace())
                        ++formatterData(currentBlock())->m_foldingIndentDelta;
                }
                if (!rule->endRegion().isEmpty()) {
                    QStack<QString> *currentRegions =
                        &formatterData(currentBlock())->m_foldingRegions;
                    if (!currentRegions->isEmpty() && rule->endRegion() == currentRegions->top()) {
                        currentRegions->pop();
                        --m_regionDepth;
                        if (progress->isClosingBraceMatchAtNonEnd())
                            --formatterData(currentBlock())->m_foldingIndentDelta;
                    }
                }
                progress->clearBracesMatches();
            }

            if (progress->isWillContinueLine()) {
                createWillContinueBlock();
                progress->setWillContinueLine(false);
            } else {
                if (rule->hasChildren())
                    iterateThroughRules(text, length, progress, true, rule->children());

                if (!rule->context().isEmpty() && contextChangeRequired(rule->context())) {
                    m_currentCaptures = progress->captures();
                    changeContext(rule->context(), rule->definition());
                    contextChanged = true;
                }
            }

            // Format is not applied to child rules directly (but relative to the offset of their
            // parent) nor to look ahead rules.
            if (!childRule && !rule->isLookAhead()) {
                if (rule->itemData().isEmpty())
                    applyFormat(startOffset, progress->offset() - startOffset,
                                m_currentContext->itemData(), m_currentContext->definition());
                else
                    applyFormat(startOffset, progress->offset() - startOffset, rule->itemData(),
                                rule->definition());
            }

            // When there is a match of one child rule the others should be skipped. Otherwise
            // the highlighting would be incorret in a case like 9ULLLULLLUULLULLUL, for example.
            if (contextChanged || childRule) {
                break;
            } else {
                it = rules.begin();
                continue;
            }
        }
        ++it;
    }

    if (!childRule && !atLeastOneMatch) {
        if (m_currentContext->isFallthrough()) {
            handleContextChange(m_currentContext->fallthroughContext(),
                                m_currentContext->definition());
            iterateThroughRules(text, length, progress, false, m_currentContext->rules());
        } else {
            applyFormat(progress->offset(), 1, m_currentContext->itemData(),
                        m_currentContext->definition());
            if (progress->isOnlySpacesSoFar() && !text.at(progress->offset()).isSpace())
                progress->setOnlySpacesSoFar(false);
            progress->incrementOffset();
        }
    }
}

bool Highlighter::contextChangeRequired(const QString &contextName) const
{
    if (contextName == kStay)
        return false;
    return true;
}

void Highlighter::changeContext(const QString &contextName,
                                const QSharedPointer<HighlightDefinition> &definition,
                                const bool assignCurrent)
{
    QString identifier = contextName;
    if (identifier.startsWith(kPop)) {
        const QStringList complexOrder = contextName.split(kExclamationMark);
        const QString orders = complexOrder.first();
        identifier = complexOrder.size() > 1 ? complexOrder[1] : QString();

        const int count = orders.splitRef(kHash, QString::SkipEmptyParts).size();
        for (int i = 0; i < count; ++i) {
            if (m_contexts.isEmpty()) {
                throw HighlighterException(
                        QCoreApplication::translate("GenericHighlighter", "Reached empty context."));
            }
            m_contexts.pop_back();
        }

        if (extractObservableState(currentBlockState()) >= PersistentsStart) {
            // One or more contexts were popped during during a persistent state.
            const QString &currentSequence = currentContextSequence();
            if (m_persistentObservableStates.contains(currentSequence))
                setCurrentBlockState(
                    computeState(m_persistentObservableStates.value(currentSequence)));
            else
                setCurrentBlockState(
                    computeState(m_leadingObservableStates.value(currentSequence)));
        }
    }
    if (!identifier.isEmpty()) {
        const QSharedPointer<Context> &context = definition->context(identifier);

        if (context->isDynamic())
            pushDynamicContext(context);
        else
            m_contexts.push_back(context);

        if (m_contexts.back()->lineEndContext() == kStay ||
            extractObservableState(currentBlockState()) >= PersistentsStart) {
            const QString &currentSequence = currentContextSequence();
            mapLeadingSequence(currentSequence);
            if (m_contexts.back()->lineEndContext() == kStay) {
                // A persistent context was pushed.
                mapPersistentSequence(currentSequence);
                setCurrentBlockState(
                    computeState(m_persistentObservableStates.value(currentSequence)));
            }
        }
    }

    if (assignCurrent)
        assignCurrentContext();
}

void Highlighter::handleContextChange(const QString &contextName,
                                      const QSharedPointer<HighlightDefinition> &definition,
                                      const bool setCurrent)
{
    if (!contextName.isEmpty() && contextChangeRequired(contextName))
        changeContext(contextName, definition, setCurrent);
}

void Highlighter::applyFormat(int offset,
                              int count,
                              const QString &itemDataName,
                              const QSharedPointer<HighlightDefinition> &definition)
{
    if (count == 0)
        return;

    QSharedPointer<ItemData> itemData;
    try {
        itemData = definition->itemData(itemDataName);
    } catch (const HighlighterException &) {
        // There are some broken files. For instance, the Printf context in java.xml points to an
        // inexistent Printf item data. These cases are considered to have normal text style.
        return;
    }

    TextFormatId formatId = kateFormatMap()->m_ids.value(itemData->style(), Normal);
    if (formatId != Normal) {
        QTextCharFormat format = formatForCategory(formatId);
        if (itemData->isCustomized()) {
            // Please notice that the following are applied every time for item data which have
            // customizations. The configureFormats function could be used to provide a "one time"
            // configuration, but it would probably require to traverse all item data from all
            // definitions available/loaded (either to set the values or for some "notifying"
            // strategy). This is because the highlighter does not really know on which
            // definition(s) it is working. Since not many item data specify customizations I
            // think this approach would fit better. If there are other ideas...
            if (itemData->color().isValid())
                format.setForeground(itemData->color());
            if (itemData->isItalicSpecified())
                format.setFontItalic(itemData->isItalic());
            if (itemData->isBoldSpecified())
                format.setFontWeight(toFontWeight(itemData->isBold()));
            if (itemData->isUnderlinedSpecified())
                format.setFontUnderline(itemData->isUnderlined());
            if (itemData->isStrikeOutSpecified())
                format.setFontStrikeOut(itemData->isStrikeOut());
        }

        setFormat(offset, count, format);
    }
}

void Highlighter::createWillContinueBlock()
{
    HighlighterCodeFormatterData *data = formatterData(currentBlock());
    const int currentObservableState = extractObservableState(currentBlockState());
    if (currentObservableState == Continued) {
        HighlighterCodeFormatterData *previousData = formatterData(currentBlock().previous());
        data->m_originalObservableState = previousData->m_originalObservableState;
    } else if (currentObservableState != WillContinue) {
        data->m_originalObservableState = currentObservableState;
    }
    const QString currentSequence = currentContextSequence();
    mapPersistentSequence(currentSequence);
    data->m_continueObservableState = m_persistentObservableStates.value(currentSequence);
    m_persistentContexts.insert(data->m_continueObservableState, m_contexts);

    setCurrentBlockState(computeState(WillContinue));
}

void Highlighter::analyseConsistencyOfWillContinueBlock(const QString &text)
{
    if (currentBlock().next().isValid() && (
        text.length() == 0 || text.at(text.length() - 1) != kBackSlash) &&
        extractObservableState(currentBlock().next().userState()) != Continued) {
        currentBlock().next().setUserState(computeState(Continued));
    }

    if (text.length() == 0 || text.at(text.length() - 1) != kBackSlash) {
        HighlighterCodeFormatterData *data = formatterData(currentBlock());
        setCurrentBlockState(computeState(data->m_originalObservableState));
    }
}

void Highlighter::mapPersistentSequence(const QString &contextSequence)
{
    if (!m_persistentObservableStates.contains(contextSequence)) {
        int newState = m_persistentObservableStatesCounter;
        m_persistentObservableStates.insert(contextSequence, newState);
        m_persistentContexts.insert(newState, m_contexts);
        ++m_persistentObservableStatesCounter;
    }
}

void Highlighter::mapLeadingSequence(const QString &contextSequence)
{
    if (!m_leadingObservableStates.contains(contextSequence))
        m_leadingObservableStates.insert(contextSequence,
                                         extractObservableState(currentBlockState()));
}

void Highlighter::pushContextSequence(int state)
{
    const QVector<QSharedPointer<Context> > &contexts = m_persistentContexts.value(state);
    for (int i = 0; i < contexts.size(); ++i)
        m_contexts.push_back(contexts.at(i));
}

QString Highlighter::currentContextSequence() const
{
    QString sequence;
    for (int i = 0; i < m_contexts.size(); ++i)
        sequence.append(m_contexts.at(i)->id());

    return sequence;
}

void Highlighter::pushDynamicContext(const QSharedPointer<Context> &baseContext)
{
    // A dynamic context is created from another context which serves as its basis. Then,
    // its rules are updated according to the captures from the calling regular expression which
    // triggered the push of the dynamic context.
    QSharedPointer<Context> context(new Context(*baseContext));
    context->configureId(m_dynamicContextsCounter);
    context->updateDynamicRules(m_currentCaptures);
    m_contexts.push_back(context);
    ++m_dynamicContextsCounter;
}

void Highlighter::assignCurrentContext()
{
    if (m_contexts.isEmpty()) {
        // This is not supposed to happen. However, there are broken files (for example, php.xml)
        // which will cause this behaviour. In such cases pushing the default context is enough to
        // keep highlighter working.
        m_contexts.push_back(m_defaultContext);
    }
    m_currentContext = m_contexts.back();
}

int Highlighter::extractRegionDepth(const int state)
{
    return state >> 12;
}

int Highlighter::extractObservableState(const int state)
{
    return state & 0xFFF;
}

int Highlighter::computeState(const int observableState) const
{
    return m_regionDepth << 12 | observableState;
}

void Highlighter::applyRegionBasedFolding() const
{
    int folding = 0;
    TextBlockUserData *currentBlockUserData = TextDocumentLayout::userData(currentBlock());
    HighlighterCodeFormatterData *data = formatterData(currentBlock());
    HighlighterCodeFormatterData *previousData = formatterData(currentBlock().previous());
    if (previousData) {
        folding = extractRegionDepth(previousBlockState());
        if (data->m_foldingIndentDelta != 0) {
            folding += data->m_foldingIndentDelta;
            if (data->m_foldingIndentDelta > 0)
                currentBlockUserData->setFoldingStartIncluded(true);
            else
                TextDocumentLayout::userData(currentBlock().previous())->setFoldingEndIncluded(false);
            data->m_foldingIndentDelta = 0;
        }
    }
    currentBlockUserData->setFoldingEndIncluded(true);
    currentBlockUserData->setFoldingIndent(folding);
}

void Highlighter::applyIndentationBasedFolding(const QString &text) const
{
    TextBlockUserData *data = TextDocumentLayout::userData(currentBlock());
    data->setFoldingEndIncluded(true);

    // If this line is empty, check its neighbours. They all might be part of the same block.
    if (text.trimmed().isEmpty()) {
        data->setFoldingIndent(0);
        const int previousIndent = neighbouringNonEmptyBlockIndent(currentBlock().previous(), true);
        if (previousIndent > 0) {
            const int nextIndent = neighbouringNonEmptyBlockIndent(currentBlock().next(), false);
            if (previousIndent == nextIndent)
                data->setFoldingIndent(previousIndent);
        }
    } else {
        data->setFoldingIndent(m_tabSettings->indentationColumn(text));
    }
}

int Highlighter::neighbouringNonEmptyBlockIndent(QTextBlock block, const bool previous) const
{
    while (true) {
        if (!block.isValid())
            return 0;
        if (block.text().trimmed().isEmpty()) {
            if (previous)
                block = block.previous();
            else
                block = block.next();
        } else {
            return m_tabSettings->indentationColumn(block.text());
        }
    }
}
