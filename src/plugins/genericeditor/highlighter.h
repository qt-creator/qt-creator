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

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QtCore/QVector>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>

#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextBlockUserData>

#include <texteditor/basetextdocumentlayout.h>

namespace GenericEditor {
namespace Internal {

class Rule;
class Context;
class HighlightDefinition;
class ProgressData;

class Highlighter : public QSyntaxHighlighter
{
public:
    Highlighter(const QSharedPointer<Context> &defaultContext, QTextDocument *parent = 0);
    virtual ~Highlighter();

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

    bool contextChangeRequired(const QString &contextName) const;
    void handleContextChange(const QString &contextName,
                             const QSharedPointer<HighlightDefinition> &definition,
                             const bool setCurrent = true);
    void changeContext(const QString &contextName,
                       const QSharedPointer<HighlightDefinition> &definition,
                       const bool setCurrent = true);

    void applyFormat(int offset,
                     int count,
                     const QString &itemData,
                     const QSharedPointer<HighlightDefinition> &definition);

    QString currentContextSequence() const;
    void mapContextSequence(const QString &contextSequence);
    void pushContextSequence(int state);
    void pushDynamicContext(const QSharedPointer<Context> &baseContext);

    void setCurrentContext();

    void createWillContinueBlock();
    void analyseConsistencyOfWillContinueBlock(const QString &text);

    struct BlockData : TextEditor::TextBlockUserData
    {
        BlockData();
        virtual ~BlockData();

        int m_originalState;
        QSharedPointer<Context> m_contextToContinue;
    };
    BlockData *initializeBlockData();

    // Block states
    // - Default [0]: Nothing special.
    // - WillContinue [1]: When there is match of the LineContinue rule (backslash as the last
    //   character).
    // - Continued [2]: Blocks that happen after a WillContinue block and continued from their
    //   context until the next line end.
    // - Persistent(s) [Anything >= 3]: Correspond to persistent contexts which last until a pop
    //   occurs due to a matching rule. Every sequence of persistent contexts seen so far is
    //   associated with a number (incremented by a unit each time).
    enum BlockState {
        Default = 0,
        WillContinue,
        Continued,
        PersistentsStart
    };
    int m_persistentStatesCounter;
    int m_dynamicContextsCounter;

    bool m_isBroken;

    QSharedPointer<Context> m_defaultContext;
    QSharedPointer<Context> m_currentContext;
    QVector<QSharedPointer<Context> > m_contexts;

    // Mapping from context sequences to the persistent state they represent.
    QHash<QString, int> m_persistentStates;
    // Mapping from context sequences to the non-persistent state that led to them.
    QHash<QString, int> m_leadingStates;
    // Mapping from persistent states to context sequences (the actual "stack").
    QHash<int, QVector<QSharedPointer<Context> > > m_persistentContexts;

    // Captures used in dynamic rules.
    QStringList m_currentCaptures;
};

} // namespace Internal
} // namespace GenericEditor

#endif // HIGHLIGHTER_H
