/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COMPLETIONSUPPORT_H
#define COMPLETIONSUPPORT_H

#include <texteditor/texteditor_global.h>

#include <QtCore/QObject>

namespace Core { class ICore; }

namespace TextEditor {

struct CompletionItem;
class ICompletionCollector;
class ITextEditable;

namespace Internal {

class CompletionWidget;

/* Completion support is responsible for querying the list of completion collectors
   and popping up the CompletionWidget with the available completions.
 */
class TEXTEDITOR_EXPORT CompletionSupport : public QObject
{
    Q_OBJECT

public:
    CompletionSupport(Core::ICore *core);

    static CompletionSupport *instance(Core::ICore *core);
public slots:
    void autoComplete(ITextEditable *editor, bool forced);

private slots:
    void performCompletion(const TextEditor::CompletionItem &item);
    void cleanupCompletions();

private:
    QList<CompletionItem> getCompletions() const;

    CompletionWidget *m_completionList;
    int m_startPosition;
    bool m_checkCompletionTrigger;          // Whether to check for completion trigger after cleanup
    ITextEditable *m_editor;
    ICompletionCollector *m_completionCollector;
};

} // namespace Internal
} // namespace TextEditor

#endif // COMPLETIONSUPPORT_H

