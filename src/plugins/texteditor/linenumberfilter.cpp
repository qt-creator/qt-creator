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

#include "linenumberfilter.h"
#include "itexteditor.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QVariant>

using namespace Core;
using namespace QuickOpen;
using namespace TextEditor;
using namespace TextEditor::Internal;

LineNumberFilter::LineNumberFilter(EditorManager *editorManager, QObject *parent):
    IQuickOpenFilter(parent)
{
    m_editorManager = editorManager;
    setShortcutString("l");
    setIncludedByDefault(true);
}

QList<FilterEntry> LineNumberFilter::matchesFor(const QString &entry)
{
    bool ok;
    QList<FilterEntry> value;
    int line = entry.toInt(&ok);
    if (line > 0 && currentTextEditor())
        value.append(FilterEntry(this, QString("Line %1").arg(line), QVariant(line)));
    return value;
}

void LineNumberFilter::accept(FilterEntry selection) const
{
    ITextEditor *editor = currentTextEditor();
    if (editor) {
        m_editorManager->ensureEditorManagerVisible();
        m_editorManager->addCurrentPositionToNavigationHistory(true);
        editor->gotoLine(selection.internalData.toInt());
        m_editorManager->addCurrentPositionToNavigationHistory();
        editor->widget()->setFocus();
    }
}

ITextEditor *LineNumberFilter::currentTextEditor() const
{
    if (!m_editorManager->currentEditor())
        return 0;
    return qobject_cast<TextEditor::ITextEditor*>(m_editorManager->currentEditor());
}
