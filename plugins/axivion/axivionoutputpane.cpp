// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "axivionoutputpane.h"

#include "axiviontr.h"

#include <utils/qtcassert.h>

#include <QStackedWidget>

namespace Axivion::Internal {

AxivionOutputPane::AxivionOutputPane(QObject *parent)
    : Core::IOutputPane(parent)
{
    m_outputWidget = new QStackedWidget;
}

AxivionOutputPane::~AxivionOutputPane()
{
    if (!m_outputWidget->parent())
        delete m_outputWidget;
}

QWidget *AxivionOutputPane::outputWidget(QWidget *parent)
{
    if (m_outputWidget)
        m_outputWidget->setParent(parent);
    else
        QTC_CHECK(false);
    return m_outputWidget;
}

QList<QWidget *> AxivionOutputPane::toolBarWidgets() const
{
    QList<QWidget *> buttons;
    return buttons;
}

QString AxivionOutputPane::displayName() const
{
    return Tr::tr("Axivion");
}

int AxivionOutputPane::priorityInStatusBar() const
{
    return -1;
}

void AxivionOutputPane::clearContents()
{
}

void AxivionOutputPane::setFocus()
{
}

bool AxivionOutputPane::hasFocus() const
{
    return false;
}

bool AxivionOutputPane::canFocus() const
{
    return true;
}

bool AxivionOutputPane::canNavigate() const
{
    return true;
}

bool AxivionOutputPane::canNext() const
{
    return false;
}

bool AxivionOutputPane::canPrevious() const
{
    return false;
}

void AxivionOutputPane::goToNext()
{
}

void AxivionOutputPane::goToPrev()
{
}

} // Axivion::Internal
