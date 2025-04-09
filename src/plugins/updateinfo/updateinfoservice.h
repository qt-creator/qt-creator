// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QString>

namespace UpdateInfo {

class Service
{
public:
    virtual ~Service() = default;

    virtual bool installPackages(const QString &filterRegex) = 0;
};

} // namespace UpdateInfo

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(UpdateInfo::Service, "UpdateInfo::Service")
QT_END_NAMESPACE
