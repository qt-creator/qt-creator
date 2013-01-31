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

#include "linenumberfilter.h"
#include "itexteditor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <QVariant>

using namespace Core;
using namespace Locator;
using namespace TextEditor;
using namespace TextEditor::Internal;

LineNumberFilter::LineNumberFilter(QObject *parent)
  : ILocatorFilter(parent)
{
    setId("Line in current document");
    setDisplayName(tr("Line in Current Document"));
    setPriority(High);
    setShortcutString(QString(QLatin1Char('l')));
    setIncludedByDefault(true);
}

QList<FilterEntry> LineNumberFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &, const QString &entry)
{
    bool ok;
    QList<FilterEntry> value;
    int line = entry.toInt(&ok);
    if (line > 0 && currentTextEditor())
        value.append(FilterEntry(this, tr("Line %1").arg(line), QVariant(line)));
    return value;
}

void LineNumberFilter::accept(FilterEntry selection) const
{
    ITextEditor *editor = currentTextEditor();
    if (editor) {
        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->addCurrentPositionToNavigationHistory();
        editor->gotoLine(selection.internalData.toInt());
        editor->widget()->setFocus();
        Core::ModeManager::activateModeType(Id(Core::Constants::MODE_EDIT_TYPE));
    }
}

ITextEditor *LineNumberFilter::currentTextEditor() const
{
    return qobject_cast<TextEditor::ITextEditor *>(EditorManager::currentEditor());
}
