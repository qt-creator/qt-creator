// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "namedwidget.h"

namespace ProjectExplorer {

NamedWidget::NamedWidget(const QString &displayName, QWidget *parent)
    : ProjectSettingsWidget(parent), m_displayName(displayName)
{
}

QString NamedWidget::displayName() const
{
    return m_displayName;
}

} // ProjectExplorer
