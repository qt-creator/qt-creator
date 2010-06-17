/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "highlighter.h"
#include "highlightdefinition.h"
#include "context.h"
#include "rule.h"
#include "itemdata.h"
#include "highlighterexception.h"
#include "progressdata.h"
#include "reuse.h"

#include <QtCore/QLatin1String>
#include <QtCore/QLatin1Char>

using namespace TextEditor;
using namespace Internal;

namespace {
    static const QLatin1String kStay("#stay");
    static const QLatin1String kPop("#pop");
    static const QLatin1Char kBackSlash('\\');
    static const QLatin1Char kHash('#');
}

const Highlighter::KateFormatMap Highlighter::m_kateFormats;

Highlighter::Highlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent),
    m_persistentStatesCounter(PersistentsStart),
    m_dynamicContextsCounter(0),
    m_isBroken(false)
{}

Highlighter::~Highlighter()
{}

Highlighter::BlockData::BlockData()
{}

Highlighter::BlockData::~BlockData()
{}

Highlighter::KateFormatMap::KateFormatMap()
{
    m_ids.insert(QLatin1String("dsNormal"), Highlighter::Normal);
    m_ids.insert(QLatin1String("dsKeyword"), Highlighter::Keyword);
    m_ids.insert(QLatin1String("dsDataType"), Highlighter::DataType);
    m_ids.insert(QLatin1String("dsDecVal"), Highlighter::Decimal);
    m_ids.insert(QLatin1String("dsBaseN"), Highlighter::BaseN);
    m_ids.insert(QLatin1String("dsFloat"), Highlighter::Float);
    m_ids.insert(QLatin1String("dsChar"), Highlighter::Char);
    m_ids.insert(QLatin1String("dsString"), Highlighter::String);
    m_ids.insert(QLatin1String("dsComment"), Highlighter::Comment);
    m_ids.insert(QLatin1String("dsOthers"), Highlighter::Others);
    m_ids.insert(QLatin1String("dsAlert"), Highlighter::Alert);
    m_ids.insert(QLatin1String("dsFunction"), Highlighter::Function);
    m_ids.insert(QLatin1String("dsRegionMarker"), Highlighter::RegionMarker);
    m_ids.insert(QLatin1String("dsError"), Highlighter::Error);
}

void Highlighter::configureFormat(TextFormatId id, const QTextCharFormat &format)
{
    m_creatorFormats[id] = format;
}

void  Highlighter::setDefaultContext(const QSharedPointer<Context> &defaultContext)
{
    m_defaultContext = defaultContext;
    m_persistentStates.insert(m_defaultContext->name(), Default);
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!m_defaultContext.isNull() && !m_isBroken) {
        try {
            setupDataForBlock(text);

            handleContextChange(m_currentContext->lineBeginContext(),
                                m_currentContext->definition());

            ProgressData progress;
            const int length = text.length();
            while (progress.offset() < length)
                iterateThroughRules(text, length, &progress, false, m_currentContext->rules());

            handleContextChange(m_currentContext->lineEndContext(),
                                m_currentContext->definition(),
                                false);
            m_contexts.clear();
        } catch (const HighlighterException &) {
            m_isBroken = true;
        }
    }

    applyVisualWhitespaceFormat(text);
}

void Highlighter::setupDataForBlock(const QString &text)
{
    if (currentBlockState() == WillContinue)
        analyseConsistencyOfWillContinueBlock(text);

    if (previousBlockState() == Default || previousBlockState() == -1)
        setupDefault();
    else if (previousBlockState() == WillContinue)
        setupFromWillContinue();
    else if (previousBlockState() == Continued)
        setupFromContinued();
    else
        setupFromPersistent();

    setCurrentContext();
}

void Highlighter::setupDefault()
{
    m_contexts.push_back(m_defaultContext);

    setCurrentBlockState(Default);
}

void Highlighter::setupFromWillContinue()
{
    BlockData *previousData = static_cast<BlockData *>(currentBlock().previous().userData());
    m_contexts.push_back(previousData->m_contextToContinue);

    if (!currentBlockUserData()) {
        BlockData *data = initializeBlockData();
        data->m_originalState = previousData->m_originalState;
    }

    if (currentBlockState() == Default || currentBlockState() == -1)
        setCurrentBlockState(Continued);
}

void Highlighter::setupFromContinued()
{
    BlockData *previousData = static_cast<BlockData *>(currentBlock().previous().userData());

    Q_ASSERT(previousData->m_originalState != WillContinue &&
             previousData->m_originalState != Continued);

    if (previousData->m_originalState == Default || previousData->m_originalState == -1)
        m_contexts.push_back(m_defaultContext);
    else
        pushContextSequence(previousData->m_originalState);

    setCurrentBlockState(previousData->m_originalState);
}

void Highlighter::setupFromPersistent()
{
    pushContextSequence(previousBlockState());

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

            if (progress->willContinueLine()) {
                createWillContinueBlock();
                progress->setWillContinueLine(false);
            } else {
                if (rule->hasChild())
                    iterateThroughRules(text, length, progress, true, rule->childs());

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
            if (progress->onlySpacesSoFar() && !text.at(progress->offset()).isSpace())
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
                                const bool setCurrent)
{
    if (contextName.startsWith(kPop)) {
        QStringList list = contextName.split(kHash, QString::SkipEmptyParts);
        for (int i = 0; i < list.size(); ++i)
            m_contexts.pop_back();

        if (currentBlockState() >= PersistentsStart) {
            // One or more contexts were popped during during a persistent state.
            const QString &currentSequence = currentContextSequence();
            if (m_persistentStates.contains(currentSequence))
                setCurrentBlockState(m_persistentStates.value(currentSequence));
            else
                setCurrentBlockState(m_leadingStates.value(currentSequence));
        }
    } else {
        const QSharedPointer<Context> &context = definition->context(contextName);

        if (context->isDynamic())
            pushDynamicContext(context);
        else
            m_contexts.push_back(context);

        if (m_contexts.back()->lineEndContext() == kStay ||
            currentBlockState() >= PersistentsStart) {
            const QString &currentSequence = currentContextSequence();
            mapLeadingSequence(currentSequence);
            if (m_contexts.back()->lineEndContext() == kStay) {
                // A persistent context was pushed.
                mapPersistentSequence(currentSequence);
                setCurrentBlockState(m_persistentStates.value(currentSequence));
            }
        }
    }

    if (setCurrent)
        setCurrentContext();
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

    TextFormatId formatId = m_kateFormats.m_ids.value(itemData->style());
    if (formatId != Normal) {
        QTextCharFormat format = m_creatorFormats.value(formatId);

        if (itemData->isCustomized()) {
            // Please notice that the following are applied every time for item datas which have
            // customizations. The configureFormats method could be used to provide a "one time"
            // configuration, but it would probably require to traverse all item datas from all
            // definitions available/loaded (either to set the values or for some "notifying"
            // strategy). This is because the highlighter does not really know on which
            // definition(s) it is working. Since not many item datas specify customizations I
            // think this approach would fit better. If there are other ideas...
            if (itemData->color().isValid())
                format.setForeground(itemData->color());
            if (itemData->isItalicSpecified())
                format.setFontItalic(itemData->isItalic());
            if (itemData->isBoldSpecified())
                format.setFontWeight(toFontWeight(itemData->isBold()));
            if (itemData->isUnderlinedSpecified())
                format.setFontUnderline(itemData->isUnderlined());
            if (itemData->isStrikedOutSpecified())
                format.setFontStrikeOut(itemData->isStrikedOut());
        }

        setFormat(offset, count, format);
    }
}

void Highlighter::applyVisualWhitespaceFormat(const QString &text)
{
    int offset = 0;
    const int length = text.length();
    while (offset < length) {
        if (text.at(offset).isSpace()) {
            int start = offset++;
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            setFormat(start, offset - start, m_creatorFormats.value(VisualWhitespace));
        } else {
            ++offset;
        }
    }
}

void Highlighter::createWillContinueBlock()
{
    if (!currentBlockUserData())
        initializeBlockData();

    BlockData *data = static_cast<BlockData *>(currentBlockUserData());
    if (currentBlockState() == Continued) {
        BlockData *previousData = static_cast<BlockData *>(currentBlock().previous().userData());
        data->m_originalState = previousData->m_originalState;
    } else if (currentBlockState() != WillContinue) {
        data->m_originalState = currentBlockState();
    }
    data->m_contextToContinue = m_currentContext;

    setCurrentBlockState(WillContinue);
}

void Highlighter::analyseConsistencyOfWillContinueBlock(const QString &text)
{
    if (currentBlock().next().isValid() && (
        text.length() == 0 || text.at(text.length() - 1) != kBackSlash) &&
        currentBlock().next().userState() != Continued) {
        currentBlock().next().setUserState(Continued);
    }

    if (text.length() == 0 || text.at(text.length() - 1) != kBackSlash) {
        BlockData *data = static_cast<BlockData *>(currentBlockUserData());
        data->m_contextToContinue.clear();
        setCurrentBlockState(data->m_originalState);
    }
}

void Highlighter::mapPersistentSequence(const QString &contextSequence)
{
    if (!m_persistentStates.contains(contextSequence)) {
        int newState = m_persistentStatesCounter;
        m_persistentStates.insert(contextSequence, newState);
        m_persistentContexts.insert(newState, m_contexts);
        ++m_persistentStatesCounter;
    }
}

void Highlighter::mapLeadingSequence(const QString &contextSequence)
{
    if (!m_leadingStates.contains(contextSequence))
        m_leadingStates.insert(contextSequence, currentBlockState());
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

Highlighter::BlockData *Highlighter::initializeBlockData()
{
    BlockData *data = new BlockData;
    setCurrentBlockUserData(data);
    return data;
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

void Highlighter::setCurrentContext()
{
    if (m_contexts.isEmpty()) {
        // This is not supposed to happen. However, there are broken files (for example, php.xml)
        // which will cause this behaviour. In such cases pushing the default context is enough to
        // keep highlighter working.
        m_contexts.push_back(m_defaultContext);
    }
    m_currentContext = m_contexts.back();
}
