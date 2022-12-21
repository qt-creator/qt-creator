// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filewizardpage.h>

namespace ProjectExplorer {

// Documentation inside.
class JsonFilePage : public Utils::FileWizardPage
{
    Q_OBJECT

public:
    JsonFilePage(QWidget *parent = nullptr);

    void initializePage() override;
    bool validatePage() override;
};

} // namespace ProjectExplorer
