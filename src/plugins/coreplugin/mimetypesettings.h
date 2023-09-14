// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dialogs/ioptionspage.h"

namespace Core::Internal {

class MimeTypeSettingsPrivate;

class MimeTypeSettings : public IOptionsPage
{
public:
    MimeTypeSettings();
    ~MimeTypeSettings() override;

    QStringList keywords() const override;

    static void restoreSettings();

private:
    MimeTypeSettingsPrivate *d;
};

} // Core::Internal
