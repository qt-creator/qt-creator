/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef COMPLETIONCOLLECTORINTERFACE_H
#define COMPLETIONCOLLECTORINTERFACE_H

#include "texteditor_global.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtGui/QIcon>

namespace TextEditor {

namespace Internal {
class ICompletionCollectorPrivate;
}

class ICompletionCollector;
class ITextEditor;
class CompletionSettings;

enum CompletionPolicy
{
    QuickFixCompletion, // Used for "Quick Fix" operation.
    TextCompletion,     // Plain word completion.
    SemanticCompletion  // Completion using code models.
};

class CompletionItem
{
public:
    CompletionItem(ICompletionCollector *collector = 0)
        : duplicateCount(0),
          order(0),
          originalIndex(0),
          collector(collector),
          isSnippet(false)
    { }

    bool isValid() const
    { return collector != 0; }

    QString text;
    QString details;
    QIcon icon;
    QVariant data;
    int duplicateCount;
    int order;
    int originalIndex;
    ICompletionCollector *collector;
    bool isSnippet;
};

/* Defines the interface to completion collectors. A completion collector tells
 * the completion support code when a completion is triggered and provides the
 * list of possible completions. It keeps an internal state so that it can be
 * polled for the list of completions, which is reset with a call to reset.
 */
class TEXTEDITOR_EXPORT ICompletionCollector : public QObject
{
    Q_OBJECT
public:
    ICompletionCollector(QObject *parent = 0);
    virtual ~ICompletionCollector();

    const CompletionSettings &completionSettings() const;

    virtual QList<CompletionItem> getCompletions();
    virtual bool shouldRestartCompletion();

    /* Returns the current active ITextEditor */
    virtual ITextEditor *editor() const = 0;
    virtual int startPosition() const = 0;

    /*
     * Returns true if this completion collector can be used with the given editor.
     */
    virtual bool supportsEditor(ITextEditor *editor) const = 0;

    /*
     * Returns true if this completion collector supports the given completion policy.
     */
    virtual bool supportsPolicy(CompletionPolicy policy) const = 0;

    /* This method should return whether the cursor is at a position which could
     * trigger an autocomplete. It will be called each time a character is typed in
     * the text editor.
     */
    virtual bool triggersCompletion(ITextEditor *editor) = 0;

    // returns starting position
    virtual int startCompletion(ITextEditor *editor) = 0;

    /* This method should add all the completions it wants to show into the list,
     * based on the given cursor position.
     */
    virtual void completions(QList<CompletionItem> *completions) = 0;

    /**
     * This method should return true when the given typed character should cause
     * the selected completion item to be completed.
     */
    virtual bool typedCharCompletes(const CompletionItem &item, QChar typedChar) = 0;

    /**
     * This method should complete the given completion item.
     *
     * \param typedChar Non-null when completion was triggered by typing a
     *                  character. Possible values depend on typedCharCompletes()
     */
    virtual void complete(const CompletionItem &item, QChar typedChar) = 0;

    /* This method gives the completion collector a chance to partially complete
     * based on a set of items. The general use case is to complete the common
     * prefix shared by all possible completion items.
     *
     * Returns whether the completion popup should be closed.
     */
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);

    virtual void sortCompletion(QList<TextEditor::CompletionItem> &completionItems);

    /* Called when it's safe to clean up the completion items.
     */
    virtual void cleanup() = 0;

    // helpers

    void filter(const QList<TextEditor::CompletionItem> &items,
                QList<TextEditor::CompletionItem> *filteredItems,
                const QString &key);

public slots:
    void setCompletionSettings(const TextEditor::CompletionSettings &);

protected:
    static bool compareChar(const QChar &item, const QChar &other);
    static bool lessThan(const QString &item, const QString &other);
    static bool completionItemLessThan(const CompletionItem &item, const CompletionItem &other);

private:
    Internal::ICompletionCollectorPrivate *m_d;
};

class TEXTEDITOR_EXPORT IQuickFixCollector : public ICompletionCollector
{
    Q_OBJECT

public:
    IQuickFixCollector(QObject *parent = 0) : ICompletionCollector(parent) {}
    virtual ~IQuickFixCollector() {}

    virtual bool typedCharCompletes(const CompletionItem &, QChar)
    { return false; }

    virtual void fix(const TextEditor::CompletionItem &item) = 0;

    virtual void complete(const CompletionItem &item, QChar typedChar)
    {
        Q_UNUSED(typedChar)
        fix(item);
    }

    virtual bool triggersCompletion(TextEditor::ITextEditor *)
    { return false; }

    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &)
    { return false; }
};


} // namespace TextEditor

#endif // COMPLETIONCOLLECTORINTERFACE_H
