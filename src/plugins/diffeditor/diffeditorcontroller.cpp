/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "diffeditorcontroller.h"

namespace DiffEditor {

DiffEditorController::DiffEditorController(QObject *parent)
    : QObject(parent),
      m_descriptionEnabled(false),
      m_descriptionVisible(true),
      m_contextLinesNumber(3),
      m_ignoreWhitespaces(true),
      m_syncScrollBars(true),
      m_currentDiffFileIndex(-1)
{
    clear();
}

DiffEditorController::~DiffEditorController()
{

}

QString DiffEditorController::clearMessage() const
{
    return m_clearMessage;
}

QList<DiffEditorController::DiffFilesContents> DiffEditorController::diffContents() const
{
    return m_diffFileList;
}

QString DiffEditorController::workingDirectory() const
{
    return m_workingDirectory;
}

QString DiffEditorController::description() const
{
    return m_description;
}

bool DiffEditorController::isDescriptionEnabled() const
{
    return m_descriptionEnabled;
}

bool DiffEditorController::isDescriptionVisible() const
{
    return m_descriptionVisible;
}

int DiffEditorController::contextLinesNumber() const
{
    return m_contextLinesNumber;
}

bool DiffEditorController::isIgnoreWhitespaces() const
{
    return m_ignoreWhitespaces;
}

bool DiffEditorController::horizontalScrollBarSynchronization() const
{
    return m_syncScrollBars;
}

int DiffEditorController::currentDiffFileIndex() const
{
    return m_currentDiffFileIndex;
}

void DiffEditorController::clear()
{
    clear(tr("No difference"));
}

void DiffEditorController::clear(const QString &message)
{
    m_clearMessage = message;
    m_currentDiffFileIndex = -1;
    emit cleared(message);
}

void DiffEditorController::setDiffContents(const QList<DiffFilesContents> &diffFileList,
                                           const QString &workingDirectory)
{
    m_diffFileList = diffFileList;
    m_workingDirectory = workingDirectory;
    m_currentDiffFileIndex = (diffFileList.isEmpty() ? -1 : 0);
    emit diffContentsChanged(diffFileList, workingDirectory);
}

void DiffEditorController::setDescription(const QString &description)
{
    if (m_description == description)
        return;

    m_description = description;
    emit descriptionChanged(description);
}

void DiffEditorController::setDescriptionEnabled(bool on)
{
    if (m_descriptionEnabled == on)
        return;

    m_descriptionEnabled = on;
    emit descriptionEnablementChanged(on);
}

void DiffEditorController::setDescriptionVisible(bool on)
{
    if (m_descriptionVisible == on)
        return;

    m_descriptionVisible = on;
    emit descriptionVisibilityChanged(on);
}

void DiffEditorController::setContextLinesNumber(int lines)
{
    const int l = qMax(lines, -1);
    if (m_contextLinesNumber == l)
        return;

    m_contextLinesNumber = l;
    emit contextLinesNumberChanged(l);
}

void DiffEditorController::setIgnoreWhitespaces(bool ignore)
{
    if (m_ignoreWhitespaces == ignore)
        return;

    m_ignoreWhitespaces = ignore;
    emit ignoreWhitespacesChanged(ignore);
}

void DiffEditorController::setHorizontalScrollBarSynchronization(bool on)
{
    if (m_syncScrollBars == on)
        return;

    m_syncScrollBars = on;
    emit horizontalScrollBarSynchronizationChanged(on);
}

void DiffEditorController::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (!m_diffFileList.count())
        return; // -1 is the only valid value in this case

    const int newIndex = qBound(0, diffFileIndex, m_diffFileList.count() - 1);

    if (m_currentDiffFileIndex == newIndex)
        return;

    m_currentDiffFileIndex = newIndex;
    emit currentDiffFileIndexChanged(newIndex);
}

} // namespace DiffEditor
