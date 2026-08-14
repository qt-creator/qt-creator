// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/secretaspect.h>

#include <utils/aspects.h>

namespace HarmonyOs::Internal {

class HarmonyOsSettings final : public Utils::AspectContainer
{
public:
    HarmonyOsSettings();

    Utils::FilePathAspect sdkLocation{this};
    Utils::BoolAspect automaticKitCreation{this};

    Utils::FilePathAspect additionalPackages{this};

    Utils::FilePathAspect signingCertificate{this};
    Utils::FilePathAspect signingProfile{this};
    Utils::FilePathAspect signingKeystore{this};
    Utils::StringAspect signingKeyAlias{this};
    Utils::StringAspect droppedPermissions{this};
    Core::SecretAspect signingKeyPassword{this};
    Core::SecretAspect signingStorePassword{this};

    // The keychain is read asynchronously while the build steps need the passwords
    // synchronously, so they are fetched once per session and kept in memory.
    QString keyPassword() const { return m_keyPassword; }
    QString storePassword() const { return m_storePassword; }

private:
    void refreshSigningPasswords();

    QString m_keyPassword;
    QString m_storePassword;
};

HarmonyOsSettings &settings();

void setupHarmonyOsSettingsPage();

} // namespace HarmonyOs::Internal
