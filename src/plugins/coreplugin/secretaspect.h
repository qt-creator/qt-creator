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

    bool isDirty() override;

    void addToLayoutImpl(Layouting::Layout &parent) override;

    void requestValue(
        const std::function<void(const Utils::expected_str<QString> &)> &callback) const;
    void setValue(const QString &value);

    void readSettings() override;
    void writeSettings() const override;

    static QString warningThatNoSecretStorageIsAvailable();

protected:
    void readSecret(const std::function<void(Utils::expected_str<QString>)> &callback) const;

private:
    std::unique_ptr<SecretAspectPrivate> d;
};

} // namespace Core
