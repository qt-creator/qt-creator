// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timestampproviderinterface.h"

#include <qmldesignercorelib_exports.h>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT TimeStampProvider : public TimeStampProviderInterface
{
public:
    Sqlite::TimeStamp timeStamp(Utils::SmallStringView name) const override;
    Sqlite::TimeStamp pause() const override { return 0; }
};

} // namespace QmlDesigner

