// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/wizard/vcsjsextension.h>

#include <QStringList>
#include <QMap>
#include <QObject>

namespace Fossil {
namespace Internal {

class FossilJsExtension : public QObject
{
    Q_OBJECT

public:
    static QMap<QString, QString> parseArgOptions(const QStringList &args);

    FossilJsExtension();
    ~FossilJsExtension();

    Q_INVOKABLE bool isConfigured() const;
    Q_INVOKABLE QString displayName() const;
    Q_INVOKABLE QString defaultAdminUser() const;
    Q_INVOKABLE QString defaultSslIdentityFile() const;
    Q_INVOKABLE QString defaultLocalRepoPath() const;
    Q_INVOKABLE bool defaultDisableAutosync() const;
};

} // namespace Internal
} // namespace Fossil
