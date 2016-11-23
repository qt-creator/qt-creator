/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "diffview.h"

#include "diffeditordocument.h"
#include "diffeditoricons.h"
#include "unifieddiffeditorwidget.h"
#include "sidebysidediffeditorwidget.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace DiffEditor {
namespace Internal {

IDiffView::IDiffView(QObject *parent) : QObject(parent), m_supportsSync(false)
{ }

QIcon IDiffView::icon() const
{
    return m_icon;
}

QString IDiffView::toolTip() const
{
    return m_toolTip;
}

bool IDiffView::supportsSync() const
{
    return m_supportsSync;
}

QString IDiffView::syncToolTip() const
{
    return m_syncToolTip;
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

void IDiffView::setSupportsSync(bool sync)
{
    m_supportsSync = sync;
}

void IDiffView::setSyncToolTip(const QString &text)
{
    m_syncToolTip = text;
}

UnifiedView::UnifiedView() : m_widget(0)
{
    setId(UNIFIED_VIEW_ID);
    setIcon(Icons::UNIFIED_DIFF.icon());
    setToolTip(QCoreApplication::translate("DiffEditor::UnifiedView", "Switch to Unified Diff Editor"));
}

QWidget *UnifiedView::widget()
{
    if (!m_widget) {
        m_widget = new UnifiedDiffEditorWidget;
        connect(m_widget, &UnifiedDiffEditorWidget::currentDiffFileIndexChanged,
                this, &UnifiedView::currentDiffFileIndexChanged);
    }
    return m_widget;
}

void UnifiedView::setDocument(DiffEditorDocument *document)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDocument(document);
    if (document && document->isReloading())
        m_widget->clear(tr("Waiting for data..."));
}

void UnifiedView::beginOperation()
{
    QTC_ASSERT(m_widget, return);
    DiffEditorDocument *document = m_widget->diffDocument();
    if (document && !document->isReloading())
        m_widget->saveState();
    m_widget->clear(tr("Waiting for data..."));
}

void UnifiedView::setDiff(const QList<FileData> &diffFileList, const QString &workingDirectory)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDiff(diffFileList, workingDirectory);
}

void UnifiedView::endOperation(bool success)
{
    QTC_ASSERT(m_widget, return);
    if (success)
        m_widget->restoreState();
    else
        m_widget->clear(tr("Failed"));
}

void UnifiedView::setCurrentDiffFileIndex(int index)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setCurrentDiffFileIndex(index);
}

void UnifiedView::setSync(bool sync)
{
    Q_UNUSED(sync);
}

SideBySideView::SideBySideView() : m_widget(0)
{
    setId(SIDE_BY_SIDE_VIEW_ID);
    setIcon(Icons::SIDEBYSIDE_DIFF.icon());
    setToolTip(QCoreApplication::translate("DiffEditor::SideBySideView",
                                           "Switch to Side By Side Diff Editor"));
    setSupportsSync(true);
    setSyncToolTip(tr("Synchronize Horizontal Scroll Bars"));
}

QWidget *SideBySideView::widget()
{
    if (!m_widget) {
        m_widget = new SideBySideDiffEditorWidget;
        connect(m_widget, &SideBySideDiffEditorWidget::currentDiffFileIndexChanged,
                this, &SideBySideView::currentDiffFileIndexChanged);
    }
    return m_widget;
}

void SideBySideView::setDocument(DiffEditorDocument *document)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDocument(document);
    if (document && document->isReloading())
        m_widget->clear(tr("Waiting for data..."));
}

void SideBySideView::beginOperation()
{
    QTC_ASSERT(m_widget, return);
    DiffEditorDocument *document = m_widget->diffDocument();
    if (document && !document->isReloading())
        m_widget->saveState();
    m_widget->clear(tr("Waiting for data..."));
}

void SideBySideView::setCurrentDiffFileIndex(int index)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setCurrentDiffFileIndex(index);
}

void SideBySideView::setDiff(const QList<FileData> &diffFileList, const QString &workingDirectory)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDiff(diffFileList, workingDirectory);
}

void SideBySideView::endOperation(bool success)
{
    QTC_ASSERT(m_widget, return);
    if (success)
        m_widget->restoreState();
    else
        m_widget->clear(tr("Failed"));
}

void SideBySideView::setSync(bool sync)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setHorizontalSync(sync);
}

} // namespace Internal
} // namespace DiffEditor
