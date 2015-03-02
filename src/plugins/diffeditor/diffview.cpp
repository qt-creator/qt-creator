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

#include "diffview.h"

#include "unifieddiffeditorwidget.h"
#include "sidebysidediffeditorwidget.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace DiffEditor {
namespace Internal {

QIcon IDiffView::icon() const
{
    return m_icon;
}

QString IDiffView::toolTip() const
{
    return m_toolTip;
}

Core::Id IDiffView::id() const
{
    return m_id;
}

void IDiffView::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void IDiffView::setToolTip(const QString &toolTip)
{
    m_toolTip = toolTip;
}

void IDiffView::setId(const Core::Id &id)
{
    m_id = id;
}

UnifiedView::UnifiedView() : m_widget(0)
{
    setId(UNIFIED_VIEW_ID);
    setIcon(QIcon(QLatin1String(":/diffeditor/images/unifieddiff.png")));
    setToolTip(QCoreApplication::translate("DiffEditor::UnifiedView", "Switch to Unified Diff Editor"));
}

QWidget *UnifiedView::widget()
{
    if (!m_widget)
        m_widget = new UnifiedDiffEditorWidget;
    return m_widget;
}

void UnifiedView::setDiffEditorGuiController(DiffEditorGuiController *controller)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDiffEditorGuiController(controller);
}

SideBySideView::SideBySideView() : m_widget(0)
{
    setId(SIDE_BY_SIDE_VIEW_ID);
    setIcon(QIcon(QLatin1String(":/diffeditor/images/sidebysidediff.png")));
    setToolTip(QCoreApplication::translate("DiffEditor::SideBySideView",
                                           "Switch to Side By Side Diff Editor"));
}

QWidget *SideBySideView::widget()
{
    if (!m_widget)
        m_widget = new SideBySideDiffEditorWidget;
    return m_widget;
}

void SideBySideView::setDiffEditorGuiController(DiffEditorGuiController *controller)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDiffEditorGuiController(controller);
}

} // namespace Internal
} // namespace DiffEditor
