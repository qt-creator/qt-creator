// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>

namespace Qdb {
namespace Internal {

class QdbQtVersion : public QtSupport::QtVersion
{
public:
    QdbQtVersion() = default;
    ~QdbQtVersion() = default;

    QString description() const final;
    QSet<Utils::Id> targetDeviceTypes() const final;
};

} // namespace Internal
} // namespace Qdb
