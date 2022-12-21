// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbqtversion.h"

#include "qdbconstants.h"

namespace Qdb {
namespace Internal {

QString QdbQtVersion::description() const
{
    return QCoreApplication::translate("QtVersion", "Boot2Qt", "Qt version is used for Boot2Qt development");
}

QSet<Utils::Id> QdbQtVersion::targetDeviceTypes() const
{
    return {Utils::Id(Constants::QdbLinuxOsType)};

}

} // namespace Internal
} // namespace Qdb
