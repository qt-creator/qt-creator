// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QTextBrowser;
QT_END_NAMESPACE

namespace WebAssembly {
namespace Internal {

class WebAssemblySettings final : public Core::PagedSettings
{
public:
    WebAssemblySettings();

    static WebAssemblySettings *instance();

    Utils::FilePathAspect emSdk;

private:
    QWidget *m_emSdkEnvGroupBox = nullptr;
    Utils::InfoLabel *m_emSdkVersionDisplay = nullptr;
    QTextBrowser *m_emSdkEnvDisplay = nullptr;
    Utils::InfoLabel *m_qtVersionDisplay = nullptr;

    void updateStatus();
};

} // namespace Internal
} // namespace WebAssmbly
