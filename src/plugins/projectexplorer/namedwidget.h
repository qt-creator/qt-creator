// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "projectsettingswidget.h"

#include <QWidget>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT NamedWidget : public ProjectSettingsWidget
{
public:
    explicit NamedWidget(const QString &displayName, QWidget *parent = nullptr);

    QString displayName() const;

private:
    QString m_displayName;
};

} // namespace ProjectExplorer
