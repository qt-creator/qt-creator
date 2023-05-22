// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/pathchooser.h>

QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace QmlDesigner {

class StudioSettingsPage : public Core::IOptionsPageWidget
{
    Q_OBJECT

public:
    void apply() final;

    StudioSettingsPage();

signals:
    void examplesDownloadPathChanged(const QString &path);
    void bundlesDownloadPathChanged(const QString &path);

private:
    QCheckBox *m_buildCheckBox;
    QCheckBox *m_debugCheckBox;
    QCheckBox *m_analyzeCheckBox;
    QCheckBox *m_toolsCheckBox;
    Utils::PathChooser *m_pathChooserExamples;
    Utils::PathChooser *m_pathChooserBundles;
};

class QMLDESIGNERBASE_EXPORT StudioConfigSettingsPage : public QObject, Core::IOptionsPage
{
    Q_OBJECT

public:
    StudioConfigSettingsPage();

signals:
    void examplesDownloadPathChanged(const QString &path);
    void bundlesDownloadPathChanged(const QString &path);
};

} // namespace QmlDesigner
