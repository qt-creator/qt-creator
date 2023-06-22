// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffview.h"

#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditoricons.h"
#include "diffeditortr.h"
#include "unifieddiffeditorwidget.h"
#include "sidebysidediffeditorwidget.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace DiffEditor::Internal {

IDiffView::IDiffView(QObject *parent) : QObject(parent)
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

Utils::Id IDiffView::id() const
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

void IDiffView::setId(const Utils::Id &id)
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

UnifiedView::UnifiedView()
{
    setId(Constants::UNIFIED_VIEW_ID);
    setIcon(Icons::UNIFIED_DIFF.icon());
    setToolTip(Tr::tr("Switch to Unified Diff Editor"));
}

QWidget *UnifiedView::widget()
{
    return textEditorWidget();
}

TextEditor::TextEditorWidget *UnifiedView::textEditorWidget()
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
    if (!document)
        return;

    switch (document->state()) {
    case DiffEditorDocument::Reloading:
        m_widget->clear(Tr::tr("Waiting for data..."));
        break;
    case DiffEditorDocument::LoadFailed:
        m_widget->clear(Tr::tr("Retrieving data failed."));
        break;
    default:
        break;
    }
}

void UnifiedView::beginOperation()
{
    QTC_ASSERT(m_widget, return);
    DiffEditorDocument *document = m_widget->diffDocument();
    if (document && document->state() == DiffEditorDocument::LoadOK)
        m_widget->saveState();
}

void UnifiedView::setDiff(const QList<FileData> &diffFileList)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDiff(diffFileList);
}

void UnifiedView::setMessage(const QString &message)
{
    m_widget->clear(message);
}

void UnifiedView::endOperation()
{
    QTC_ASSERT(m_widget, return);
    m_widget->restoreState();
}

void UnifiedView::setCurrentDiffFileIndex(int index)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setCurrentDiffFileIndex(index);
}

void UnifiedView::setSync(bool sync)
{
    Q_UNUSED(sync)
}

SideBySideView::SideBySideView()
    : m_widget(nullptr)
{
    setId(Constants::SIDE_BY_SIDE_VIEW_ID);
    setIcon(Icons::SIDEBYSIDE_DIFF.icon());
    setToolTip(Tr::tr("Switch to Side By Side Diff Editor"));
    setSupportsSync(true);
    setSyncToolTip(Tr::tr("Synchronize Horizontal Scroll Bars"));
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

TextEditor::TextEditorWidget *SideBySideView::sideEditorWidget(DiffSide side)
{
    widget(); // ensure widget creation
    return m_widget->sideEditorWidget(side);
}

void SideBySideView::setDocument(DiffEditorDocument *document)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDocument(document);
    if (!document)
        return;

    switch (document->state()) {
    case DiffEditorDocument::Reloading:
        m_widget->clear(Tr::tr("Waiting for data..."));
        break;
    case DiffEditorDocument::LoadFailed:
        m_widget->clear(Tr::tr("Retrieving data failed."));
        break;
    default:
        break;
    }
}

void SideBySideView::beginOperation()
{
    QTC_ASSERT(m_widget, return);
    DiffEditorDocument *document = m_widget->diffDocument();
    if (document && document->state() == DiffEditorDocument::LoadOK)
        m_widget->saveState();
}

void SideBySideView::setCurrentDiffFileIndex(int index)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setCurrentDiffFileIndex(index);
}

void SideBySideView::setDiff(const QList<FileData> &diffFileList)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setDiff(diffFileList);
}

void SideBySideView::setMessage(const QString &message)
{
    QTC_ASSERT(m_widget, return);
    m_widget->clear(message);
}

void SideBySideView::endOperation()
{
    QTC_ASSERT(m_widget, return);
    m_widget->restoreState();
}

void SideBySideView::setSync(bool sync)
{
    QTC_ASSERT(m_widget, return);
    m_widget->setHorizontalSync(sync);
}

} // namespace DiffEditor::Internal
