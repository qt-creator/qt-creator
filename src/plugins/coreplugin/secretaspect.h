// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/aspects.h>

namespace Core {
class SecretAspectPrivate;

class CORE_EXPORT SecretAspect : public Utils::BaseAspect
{
public:
    using valueType = QString;

    static bool isSecretStorageAvailable();

    explicit SecretAspect(Utils::AspectContainer *container = nullptr);
    ~SecretAspect() override;

    bool isDirty() const override;

    void addToLayoutImpl(Layouting::Layout &parent) override;

    void requestValue(
        const std::function<void(const Utils::Result<QString> &)> &callback) const;
    void setValue(const QString &value);

    // The keychain service and key. When unset, both are derived from the
    // settings key by splitting it at the last '.'.
    void setService(const QString &service);
    void setKey(const QString &key);

    void readSettings() override;
    void writeSettings() const override;

    static QString warningThatNoSecretStorageIsAvailable();

protected:
    void readSecret(const std::function<void(Utils::Result<QString>)> &callback) const;

private:
    std::unique_ptr<SecretAspectPrivate> d;
};

/*!
    Removes the secret stored under \a service and \a key, from the keychain as
    well as from the plaintext fallback in the settings.

    Safe to call while the object owning the corresponding aspect is being
    destroyed, as the keychain job does not depend on it.
*/
CORE_EXPORT void deleteSecret(const QString &service, const QString &key);

} // namespace Core
