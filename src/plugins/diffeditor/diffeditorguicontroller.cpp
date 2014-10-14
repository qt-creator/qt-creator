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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diffeditorguicontroller.h"
#include "diffeditorcontroller.h"

#include <coreplugin/icore.h>

static const char settingsGroupC[] = "DiffEditor";
static const char descriptionVisibleKeyC[] = "DescriptionVisible";
static const char horizontalScrollBarSynchronizationKeyC[] =
        "HorizontalScrollBarSynchronization";

namespace DiffEditor {

DiffEditorGuiController::DiffEditorGuiController(
        DiffEditorController *controller,
        QObject *parent)
    : QObject(parent),
      m_controller(controller),
      m_descriptionVisible(true),
      m_syncScrollBars(true),
      m_currentDiffFileIndex(-1)
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    m_descriptionVisible = s->value(QLatin1String(descriptionVisibleKeyC),
                                    m_descriptionVisible).toBool();
    m_syncScrollBars = s->value(QLatin1String(horizontalScrollBarSynchronizationKeyC),
                                m_syncScrollBars).toBool();
    s->endGroup();

    connect(m_controller, SIGNAL(cleared(QString)),
            this, SLOT(slotUpdateDiffFileIndex()));
    connect(m_controller, SIGNAL(diffFilesChanged(QList<FileData>,QString)),
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
    m_currentDiffFileIndex = (m_controller->diffFiles().isEmpty() ? -1 : 0);
}

void DiffEditorGuiController::setDescriptionVisible(bool on)
{
    if (m_descriptionVisible == on)
        return;

    m_descriptionVisible = on;

    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(descriptionVisibleKeyC), m_descriptionVisible);
    s->endGroup();

    emit descriptionVisibilityChanged(on);
}

void DiffEditorGuiController::setHorizontalScrollBarSynchronization(bool on)
{
    if (m_syncScrollBars == on)
        return;

    m_syncScrollBars = on;

    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(horizontalScrollBarSynchronizationKeyC),
                m_syncScrollBars);
    s->endGroup();

    emit horizontalScrollBarSynchronizationChanged(on);
}

void DiffEditorGuiController::setCurrentDiffFileIndex(int diffFileIndex)
{
    if (m_controller->diffFiles().isEmpty())
        return; // -1 is the only valid value in this case

    const int newIndex = qBound(0, diffFileIndex,
                                m_controller->diffFiles().count() - 1);

    if (m_currentDiffFileIndex == newIndex)
        return;

    m_currentDiffFileIndex = newIndex;
    emit currentDiffFileIndexChanged(newIndex);
}

} // namespace DiffEditor
