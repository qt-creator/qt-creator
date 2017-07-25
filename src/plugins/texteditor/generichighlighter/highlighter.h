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

#pragma once

#include "context.h"

// Yes, this is correct. These are found somewhere else when building the autotest.
#include <textdocumentlayout.h>
#include <syntaxhighlighter.h>

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
class HighlightDefinition;
class ProgressData;

} // namespace Internal

/*
  Warning: Due to a very ugly hack with generichighlighter test
  you can't export this class, so that it would be used from
  other plugins. That's why highlighterutils.h was introduced.
*/
class Highlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument *parent = 0);
    virtual ~Highlighter();

    enum TextFormatId {
        Normal,
        Keyword,
        DataType,
        Comment,
        Decimal,
        BaseN,
        Float,
        Char,
        SpecialChar,
        String,
        Alert,
        Information,
        Warning,
        Error,
        Function,
        RegionMarker,
        BuiltIn,
        Extension,
        Operator,
        Variable,
        Attribute,
        Annotation,
        CommentVar,
        Import,
        Others,
        Identifier,
        Documentation,
        ControlFlow,
        Preprocessor,
        VerbatimString,
        SpecialString,
        Constant,
        TextFormatIdCount
    };

    void setTabSettings(const TabSettings &ts);
    void setDefaultContext(const QSharedPointer<Internal::Context> &defaultContext);

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
                             Internal::ProgressData *progress,
                             const bool childRule,
                             const QList<QSharedPointer<Internal::Rule> > &rules);

    void assignCurrentContext();
    bool contextChangeRequired(const QString &contextName) const;
    void handleContextChange(const QString &contextName,
                             const QSharedPointer<Internal::HighlightDefinition> &definition,
                             const bool setCurrent = true);
    void changeContext(const QString &contextName,
                       const QSharedPointer<Internal::HighlightDefinition> &definition,
                       const bool assignCurrent = true);

    QString currentContextSequence() const;
    void mapPersistentSequence(const QString &contextSequence);
    void mapLeadingSequence(const QString &contextSequence);
    void pushContextSequence(int state);

    void pushDynamicContext(const QSharedPointer<Internal::Context> &baseContext);

    void createWillContinueBlock();
    void analyseConsistencyOfWillContinueBlock(const QString &text);

    void applyFormat(int offset,
                     int count,
                     const QString &itemDataName,
                     const QSharedPointer<Internal::HighlightDefinition> &definition);

    void applyRegionBasedFolding() const;
    void applyIndentationBasedFolding(const QString &text) const;
    int neighbouringNonEmptyBlockIndent(QTextBlock block, const bool previous) const;

    static TextBlockUserData *blockData(QTextBlockUserData *userData);

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

    QSharedPointer<Internal::Context> m_defaultContext;
    QSharedPointer<Internal::Context> m_currentContext;
    QVector<QSharedPointer<Internal::Context> > m_contexts;

    // Mapping from context sequences to the observable persistent state they represent.
    QHash<QString, int> m_persistentObservableStates;
    // Mapping from context sequences to the non-persistent observable state that led to them.
    QHash<QString, int> m_leadingObservableStates;
    // Mapping from observable persistent states to context sequences (the actual "stack").
    QHash<int, QVector<QSharedPointer<Internal::Context> > > m_persistentContexts;

    // Captures used in dynamic rules.
    QStringList m_currentCaptures;
};

} // namespace TextEditor
