// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtwizard.h"

namespace QmakeProjectManager {
namespace Internal {

struct QtProjectParameters;

class SubdirsProjectWizardDialog : public BaseQmakeProjectWizardDialog
{
    Q_OBJECT
public:
    explicit SubdirsProjectWizardDialog(const Core::BaseFileWizardFactory *factory, const QString &templateName,
                                    const QIcon &icon,
                                    QWidget *parent,
                                    const Core::WizardDialogParameters &parameters);

    QtProjectParameters parameters() const;
};

} // namespace Internal
} // namespace QmakeProjectManager
