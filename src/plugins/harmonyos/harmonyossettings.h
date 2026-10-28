// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/secretaspect.h>

#include <utils/aspects.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

namespace HarmonyOs::Internal {

class HarmonyOsSettings final : public Utils::AspectContainer
{
public:
    HarmonyOsSettings();

    Utils::FilePathAspect sdkLocation{this};
    Utils::BoolAspect automaticKitCreation{this};

    Utils::FilePathAspect additionalPackages{this};
    Utils::FilePathAspect deviceSdkLocation{this};

    Utils::StringAspect qtRepository{this};

    Utils::BoolAspect runWithoutInstalling{this};

    Utils::FilePathAspect signingCertificate{this};
    Utils::FilePathAspect signingProfile{this};
    Utils::FilePathAspect signingKeystore{this};
    Utils::StringAspect signingKeyAlias{this};
    Core::SecretAspect signingKeyPassword{this};
    Core::SecretAspect signingStorePassword{this};

private:
    QtTaskTree::QSingleTaskTreeRunner m_sdkDownloader;
    QtTaskTree::QSingleTaskTreeRunner m_qtDownloader;
    QtTaskTree::QSingleTaskTreeRunner m_qtToolsAdder;
};

HarmonyOsSettings &settings();

void setupHarmonyOsSettingsPage();

} // namespace HarmonyOs::Internal
