/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include "basetextdocumentlayout.h"
#include "syntaxhighlighter.h"

#include <QString>
#include <QVector>
#include <QStack>
#include <QSharedPointer>
#include <QStringList>

#include <QTextCharFormat>

namespace TextEditor {

class TabSettings;

namespace Internal {

class Rule;
class Context;
class HighlightDefinition;
class ProgressData;

class Highlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);
    virtual ~Highlighter();

    enum TextFormatId {
        Normal,
        VisualWhitespace,
        Keyword,
        DataType,
        Decimal,
        BaseN,
        Float,
        Char,
        String,
        Comment,
        Alert,
        Error,
        Function,
        RegionMarker,
        Others
    };

    void configureFormat(TextFormatId id, const QTextCharFormat &format);
    void setTabSettings(const TabSettings &ts);
    void setDefaultContext(const QSharedPointer<Context> &defaultContext);

protected:
    virtual void highlightBlock(const QString &text);

private:

    void setupDataForBlock(const QString &text);
    void setupDefault();
    void setupFromWillContinue();
    void setupFromContinued();
    void setupFromPersistent();

    void iterateThroughRules(const QString &text,
                             const int length,
                             ProgressData *progress,
                             const bool childRule,
                             const QList<QSharedPointer<Rule> > &rules);

    void assignCurrentContext();
    bool contextChangeRequired(const QString &contextName) const;
    void handleContextChange(const QString &contextName,
                             const QSharedPointer<HighlightDefinition> &definition,
                             const bool setCurrent = true);
    void changeContext(const QString &contextName,
                       const QSharedPointer<HighlightDefinition> &definition,
                       const bool assignCurrent = true);

    QString currentContextSequence() const;
    void mapPersistentSequence(const QString &contextSequence);
    void mapLeadingSequence(const QString &contextSequence);
    void pushContextSequence(int state);

    void pushDynamicContext(const QSharedPointer<Context> &baseContext);

    void createWillContinueBlock();
    void analyseConsistencyOfWillContinueBlock(const QString &text);

    void applyFormat(int offset,
                     int count,
                     const QString &itemDataName,
                     const QSharedPointer<HighlightDefinition> &definition);

    void applyRegionBasedFolding() const;
    void applyIndentationBasedFolding(const QString &text) const;
    int neighbouringNonEmptyBlockIndent(QTextBlock block, const bool previous) const;

    // Mapping from Kate format strings to format ids.
    struct KateFormatMap
    {
        KateFormatMap();
        QHash<QString, TextFormatId> m_ids;
    };
    static const KateFormatMap m_kateFormats;
    QHash<TextFormatId, QTextCharFormat> m_creatorFormats;

    struct BlockData : TextBlockUserData
    {
        BlockData();
        virtual ~BlockData();

        int m_foldingIndentDelta;
        int m_originalObservableState;
        QStack<QString> m_foldingRegions;
        QSharedPointer<Context> m_contextToContinue;
    };
    BlockData *initializeBlockData();
    static BlockData *blockData(QTextBlockUserData *userData);

    // Block states are composed by the region depth (used for code folding) and what I call
    // observable states. Observable states occupy the 12 least significant bits. They might have
    // the following values:
    // - Default [0]: Nothing special.
    // - WillContinue [1]: When there is match of the LineContinue rule (backslash as the last
    //   character).
    // - Continued [2]: Blocks that happen after a WillContinue block and continue from their
    //   context until the next line end.
    // - Persistent(s) [Anything >= 3]: Correspond to persistent contexts which last until a pop
    //   occurs due to a matching rule. Every sequence of persistent contexts seen so far is
    //   associated with a number (incremented by a unit each time).
    // Region depths occupy the remaining bits.
    enum ObservableBlockState {
        Default = 0,
        WillContinue,
        Continued,
        PersistentsStart
    };
    int computeState(const int observableState) const;

    static int extractRegionDepth(const int state);
    static int extractObservableState(const int state);

    int m_regionDepth;
    bool m_indentationBasedFolding;
    const TabSettings *m_tabSettings;

    int m_persistentObservableStatesCounter;
    int m_dynamicContextsCounter;

    bool m_isBroken;

    QSharedPointer<Context> m_defaultContext;
    QSharedPointer<Context> m_currentContext;
    QVector<QSharedPointer<Context> > m_contexts;

    // Mapping from context sequences to the observable persistent state they represent.
    QHash<QString, int> m_persistentObservableStates;
    // Mapping from context sequences to the non-persistent observable state that led to them.
    QHash<QString, int> m_leadingObservableStates;
    // Mapping from observable persistent states to context sequences (the actual "stack").
    QHash<int, QVector<QSharedPointer<Context> > > m_persistentContexts;

    // Captures used in dynamic rules.
    QStringList m_currentCaptures;
};

} // namespace Internal
} // namespace TextEditor

#endif // HIGHLIGHTER_H
