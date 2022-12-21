// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QMetaType>

namespace Designer {
/* Parameters passed to generate the code part of a form class.
 * Code generation using the parameters is provided as an invokable
 * service by QtDesignerFormClassCodeGenerator.
 * Thus, the parameters class must not have linkage. */

class FormClassWizardParameters
{
public:
    QString uiTemplate;
    QString className;
    Utils::FilePath path;
    QString sourceFile;
    QString headerFile;
    QString uiFile;
    bool usePragmaOnce = false;
};
} // namespace Designer

Q_DECLARE_METATYPE(Designer::FormClassWizardParameters)
