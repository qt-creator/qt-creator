/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef COMPLETIONCOLLECTORINTERFACE_H
#define COMPLETIONCOLLECTORINTERFACE_H

#include "texteditor_global.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtGui/QKeyEvent>

namespace TextEditor {

class ICompletionCollector;
class ITextEditable;

struct CompletionItem
{
    CompletionItem(ICompletionCollector *collector = 0)
        : m_relevance(0),
          m_collector(collector)
    { }

    bool isValid() const
    { return m_collector != 0; }

    operator bool() const
    { return m_collector != 0; }

    QString m_text;
    QString m_details;
    QIcon m_icon;
    QVariant m_data;
    int m_relevance;
    ICompletionCollector *m_collector;
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
    ICompletionCollector(QObject *parent = 0) : QObject(parent) {}
    virtual ~ICompletionCollector() {}

    /* This method should return whether the cursor is at a position which could
     * trigger an autocomplete. It will be called each time a character is typed in
     * the text editor.
     */
    virtual bool triggersCompletion(ITextEditable *editor) = 0;

    // returns starting position
    virtual int startCompletion(ITextEditable *editor) = 0;

    /* This method should add all the completions it wants to show into the list,
     * based on the given cursor position.
     */
    virtual void completions(QList<CompletionItem> *completions) = 0;

    /* This method should complete the given completion item.
     */
    virtual void complete(const CompletionItem &item) = 0;

    /* This method gives the completion collector a chance to partially complete
     * based on a set of items. The general use case is to complete the common
     * prefix shared by all possible completion items.
     *
     * Returns whether the completion popup should be closed.
     */
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems) = 0;

    /* Called when it's safe to clean up the completion items.
     */
    virtual void cleanup() = 0;
};

} // namespace TextEditor

#endif // COMPLETIONCOLLECTORINTERFACE_H
