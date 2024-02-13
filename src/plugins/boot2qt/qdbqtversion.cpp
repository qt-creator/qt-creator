// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbqtversion.h"

#include "qdbconstants.h"
#include "qdbtr.h"

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtsupporttr.h>

namespace Qdb::Internal {

class QdbQtVersion final : public QtSupport::QtVersion
{
public:
    QString description() const final
    {
        return QtSupport::Tr::tr("Boot2Qt", "Qt version is used for Boot2Qt development");
    }
    QSet<Utils::Id> targetDeviceTypes() const final
    {
        return {Utils::Id(Constants::QdbLinuxOsType)};
    }
};

class QdbQtVersionFactory final : public QtSupport::QtVersionFactory
{
public:
    QdbQtVersionFactory()
    {
        setQtVersionCreator([] { return new QdbQtVersion; });
        setSupportedType("Qdb.EmbeddedLinuxQt");
        setPriority(99);
        setRestrictionChecker([](const SetupData &setup) {
            return setup.platforms.contains("boot2qt");
        });
    }
};

void setupQdbQtVersion()
{
    static QdbQtVersionFactory theQdbQtVersionFactory;
}

} // Qdb::Internal
