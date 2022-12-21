// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QVariant>
#include <QObject>

namespace Designer {
class FormClassWizardParameters;

// Publicly registered service to generate the code for a form class
// (See PluginManager::Invoke) to be accessed by Qt4ProjectManager.
class QtDesignerFormClassCodeGenerator : public QObject
{
    Q_OBJECT
public:
    QtDesignerFormClassCodeGenerator();
    ~QtDesignerFormClassCodeGenerator() override;

    static bool generateCpp(const FormClassWizardParameters &parameters,
                            QString *header, QString *source, int indentation = 4);

    // Generate form class code according to settings.
    Q_INVOKABLE QVariant generateFormClassCode(const Designer::FormClassWizardParameters &parameters);
};
} // namespace Designer
