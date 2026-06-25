// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainsidebar.h"

#include <profiler/profilertr.h>

#include <utils/filepath.h>
#include <utils/stylehelper.h>
#include <utils/widgets.h>

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

using namespace Profiler;
using namespace Utils;
using namespace Utils::StyleHelper;

namespace QtProfiler {

MainSidebar::MainSidebar(QWidget *parent)
    : QWidget(parent)
{
    auto titleBar = new Utils::StyledBar;
    auto titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(SpacingTokens::PaddingHXs, 0, SpacingTokens::PaddingHXs, 0);
    titleBarLayout->addWidget(new QLabel(Tr::tr("Traces")));

    m_list = new QListWidget;
    m_list->setFrameShape(QFrame::NoFrame);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(titleBar);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current) {
        if (current)
            emit traceActivated(FilePath::fromVariant(current->data(Qt::UserRole)));
    });
}

void MainSidebar::addTrace(const FilePath &filePath)
{
    const QVariant data = filePath.toVariant();

    // Programmatic selection must not trigger a reload, so block currentItemChanged.
    const QSignalBlocker blocker(m_list);

    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *item = m_list->item(i);
        if (item->data(Qt::UserRole) == data) {
            m_list->setCurrentItem(item);
            return;
        }
    }

    auto item = new QListWidgetItem(filePath.fileName());
    item->setToolTip(filePath.toUserOutput());
    item->setData(Qt::UserRole, data);
    m_list->addItem(item);
    m_list->setCurrentItem(item);
}

bool MainSidebar::removeCurrentTrace()
{
    QListWidgetItem *current = m_list->currentItem();
    if (!current)
        return false;

    // takeItem() drops the row and selects a neighbour, emitting currentItemChanged
    // (and thus traceActivated) for the new selection, or nullptr if none remain.
    delete m_list->takeItem(m_list->row(current));
    return m_list->currentItem() != nullptr;
}

FilePath MainSidebar::currentTrace() const
{
    if (QListWidgetItem *item = m_list->currentItem())
        return FilePath::fromVariant(item->data(Qt::UserRole));
    return {};
}

} // namespace QtProfiler
