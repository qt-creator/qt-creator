// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

namespace Core::JsBind { class Registrar; }

namespace Fossil::Internal {

class FossilJsExtension
{
public:
    static QMap<QString, QString> parseArgOptions(const QStringList &args);

    bool isConfigured() const;
    QString displayName() const;
    QString defaultAdminUser() const;
    QString defaultSslIdentityFile() const;
    QString defaultLocalRepoPath() const;
    bool defaultDisableAutosync() const;
};

// Registers the functions of the "Fossil" JavaScript object.
void registerFossilFunctions(Core::JsBind::Registrar &registrar);

} // namespace Fossil::Internal
