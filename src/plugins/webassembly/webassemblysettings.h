// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

QT_BEGIN_NAMESPACE
class QTextBrowser;
QT_END_NAMESPACE

namespace Utils { class InfoLabel; }

namespace WebAssembly::Internal {

class WebAssemblySettings final : public Utils::AspectContainer
{
public:
    WebAssemblySettings();

    Utils::FilePathAspect emSdk{this};

private:
    QWidget *m_emSdkEnvGroupBox = nullptr;
    Utils::InfoLabel *m_emSdkVersionDisplay = nullptr;
    QTextBrowser *m_emSdkEnvDisplay = nullptr;
    Utils::InfoLabel *m_qtVersionDisplay = nullptr;

    void updateStatus();
};

WebAssemblySettings &settings();

} // WebAssmbly::Internal
