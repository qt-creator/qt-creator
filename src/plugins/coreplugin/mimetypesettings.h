// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Core::Internal {

class MimeTypeSettingsPrivate;

class MimeTypeSettings : public IOptionsPage
{
    Q_OBJECT

public:
    MimeTypeSettings();
    ~MimeTypeSettings() override;

    QWidget *widget() override;
    void apply() override;
    void finish() override;

    static void restoreSettings();
private:
    MimeTypeSettingsPrivate *d;
};

} // Core::Internal
