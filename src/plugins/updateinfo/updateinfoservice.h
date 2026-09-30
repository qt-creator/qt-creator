// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QObject>
#include <QString>

namespace UpdateInfo {

inline constexpr char SERVICE_SCHEME[] = "updateinfo";
inline constexpr char SERVICE_URL[] = "updateinfo://install?";

class Service
{
public:
    virtual ~Service() = default;

    virtual bool installPackages(const QString &filterRegex) = 0;

    // The directory the Qt installer manages, i.e. the one holding the
    // maintenance tool and a subdirectory per installed Qt version.
    virtual Utils::FilePath installationRoot() const = 0;
};

} // namespace UpdateInfo

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(UpdateInfo::Service, "UpdateInfo::Service")
QT_END_NAMESPACE
