// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sqlitetimestamp.h>
#include <utils/smallstringview.h>

#include <QImage>

namespace QmlDesigner {

class TimeStampProviderInterface
{
public:
    virtual Sqlite::TimeStamp timeStamp(Utils::SmallStringView name) const = 0;
    virtual Sqlite::TimeStamp pause() const = 0;

protected:
    ~TimeStampProviderInterface() = default;
};

} // namespace QmlDesigner
