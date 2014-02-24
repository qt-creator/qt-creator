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

#include "diffeditorguicontroller.h"
#include "diffeditorcontroller.h"

namespace DiffEditor {

DiffEditorGuiController::DiffEditorGuiController(DiffEditorController *controller, QObject *parent)
    : QObject(parent),
      m_controller(controller),
      m_descriptionVisible(true),
      m_contextLinesNumber(3),
      m_ignoreWhitespaces(true),
      m_syncScrollBars(true),
      m_currentDiffFileIndex(-1)
{
    connect(m_controller, SIGNAL(cleared(QString)), this, SLOT(slotUpdateDiffFileIndex()));
    connect(m_controller, SIGNAL(diffContentsChanged(QList<DiffEditorController::DiffFilesContents>,QString)),
            this, SLOT(slotUpdateDiffFileIndex()));
    slotUpdateDiffFileIndex();
}

DiffEditorGuiController::~DiffEditorGuiController()
{

}

DiffEditorController *DiffEditorGuiController::controller() const
{
    return m_controller;
}

bool DiffEditorGuiController::isDescriptionVisible() const
{
    return m_descriptionVisible;
}

int DiffEditorGuiController::contextLinesNumber() const
{
    return m_contextLinesNumber;
}

bool DiffEditorGuiController::isIgnoreWhitespaces() const
{
    return m_ignoreWhitespaces;
}

bool DiffEditorGuiController::horizontalScrollBarSynchronization() const
{
    return m_syncScrollBars;
}

int DiffEditorGuiController::currentDiffFileIndex() const
{
    return m_currentDiffFileIndex;
}

void DiffEditorGuiController::slotUpdateDiffFileIndex()
{
    m_currentDiffFileIndex = (m_controller->diffContents().isEmpty() ? -1 : 0);
}

void DiffEditorGuiController::setDescriptionVisible(bool on)
{
    if (m_descriptionVisible == on)
        return;

    m_descriptionVisible = on;
    emit descriptionVisibilityChanged(on);
}

void DiffEditorGuiController::setContextLinesNumber(int lines)
{
    const int l = qMax(lines, -1);
    if (m_contextLinesNumber == l)
        return;

    m_contextLinesNumber = l;
    emit contextLinesNumberChanged(l);
}

void DiffEditorGuiController::setIgnoreWhitespaces(bool ignore)
{
    if (m_ignoreWhitespaces == ignore)
        return;

    m_ignoreWhitespaces = ignore;
    emit ignoreWhitespacesChanged(ignore);
}

void DiffEditorGuiController::setHorizontalScrollBarSynchronization(bool on)
{
    if (m_syncScrollBars == on)
        return;

    m_syncScrollBars = on;
    emit horizontalScrollBarSynchronizationChanged(on);
}

void DiffEditorGuiController::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_controller->diffContents().isEmpty())
        return; // -1 is the only valid value in this case

    const int newIndex = qBound(0, diffFileIndex, m_controller->diffContents().count() - 1);

    if (m_currentDiffFileIndex == newIndex)
        return;

    m_currentDiffFileIndex = newIndex;
    emit currentDiffFileIndexChanged(newIndex);
}

} // namespace DiffEditor
