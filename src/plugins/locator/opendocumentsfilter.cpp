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

#include "opendocumentsfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QDir>

using namespace Core;
using namespace Locator;
using namespace Locator::Internal;
using namespace Utils;

OpenDocumentsFilter::OpenDocumentsFilter(EditorManager *editorManager) :
    m_editorManager(editorManager)
{
    setId("Open documents");
    setDisplayName(tr("Open Documents"));
    setShortcutString(QString(QLatin1Char('o')));
    setIncludedByDefault(true);

    connect(m_editorManager, SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(refreshInternally()));
    connect(m_editorManager, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(refreshInternally()));
}

QList<FilterEntry> OpenDocumentsFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry_)
{
    QList<FilterEntry> value;
    QString entry = entry_;
    const QString lineNoSuffix = EditorManager::splitLineNumber(&entry);
    const QChar asterisk = QLatin1Char('*');
    QString pattern = QString(asterisk);
    pattern += entry;
    pattern += asterisk;
    QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return value;
    foreach (const OpenEditorsModel::Entry &editorEntry, m_editors) {
        if (future.isCanceled())
            break;
        QString fileName = editorEntry.fileName();
        QString displayName = editorEntry.displayName();
        if (regexp.exactMatch(displayName)) {
            if (!fileName.isEmpty()) {
                QFileInfo fi(fileName);
                FilterEntry fiEntry(this, fi.fileName(), QString(fileName + lineNoSuffix));
                fiEntry.extraInfo = FileUtils::shortNativePath(FileName(fi));
                fiEntry.fileName = fileName;
                value.append(fiEntry);
            }
        }
    }
    return value;
}

void OpenDocumentsFilter::refreshInternally()
{
    m_editors.clear();
    foreach (IEditor *editor, m_editorManager->openedEditors()) {
        OpenEditorsModel::Entry entry;
        // don't work on IEditor directly, since that will be useless with split windows
        entry.m_displayName = editor->displayName();
        entry.m_fileName = editor->document()->fileName();
        m_editors.append(entry);
    }
    m_editors += m_editorManager->openedEditorsModel()->restoredEditors();
}

void OpenDocumentsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, "refreshInternally", Qt::BlockingQueuedConnection);
}

void OpenDocumentsFilter::accept(FilterEntry selection) const
{
    EditorManager::openEditor(selection.internalData.toString(), Id(),
                              EditorManager::ModeSwitch | EditorManager::CanContainLineNumber);
}
