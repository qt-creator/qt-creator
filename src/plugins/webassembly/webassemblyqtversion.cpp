// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "webassemblyconstants.h"
#include "webassemblyqtversion.h"
#include "webassemblytr.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <remotelinux/remotelinux_constants.h>
#include <coreplugin/featureprovider.h>
#include <coreplugin/icore.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QVersionNumber>

using namespace QtSupport;
using namespace Utils;

namespace WebAssembly {
namespace Internal {

WebAssemblyQtVersion::WebAssemblyQtVersion() = default;

QString WebAssemblyQtVersion::description() const
{
    return Tr::tr("WebAssembly", "Qt Version is meant for WebAssembly");
}

QSet<Id> WebAssemblyQtVersion::targetDeviceTypes() const
{
    return {Constants::WEBASSEMBLY_DEVICE_TYPE};
}

WebAssemblyQtVersionFactory::WebAssemblyQtVersionFactory()
{
    setQtVersionCreator([] { return new WebAssemblyQtVersion; });
    setSupportedType(Constants::WEBASSEMBLY_QT_VERSION);
    setPriority(1);
    setRestrictionChecker([](const SetupData &setup) {
        return setup.platforms.contains("wasm");
    });
}

bool WebAssemblyQtVersion::isValid() const
{
    return QtVersion::isValid() && qtVersion() >= minimumSupportedQtVersion();
}

QString WebAssemblyQtVersion::invalidReason() const
{
    const QString baseReason = QtVersion::invalidReason();
    if (!baseReason.isEmpty())
        return baseReason;

    return Tr::tr("%1 does not support Qt for WebAssembly below version %2.")
            .arg(Core::ICore::versionString())
            .arg(minimumSupportedQtVersion().toString());
}

const QVersionNumber &WebAssemblyQtVersion::minimumSupportedQtVersion()
{
    const static QVersionNumber number(5, 15);
    return number;
}

bool WebAssemblyQtVersion::isQtVersionInstalled()
{
    return anyOf(QtVersionManager::versions(), [](const QtVersion *v) {
        return v->type() == Constants::WEBASSEMBLY_QT_VERSION;
    });
}

bool WebAssemblyQtVersion::isUnsupportedQtVersionInstalled()
{
    return anyOf(QtVersionManager::versions(), [](const QtVersion *v) {
        return v->type() == Constants::WEBASSEMBLY_QT_VERSION
                && v->qtVersion() < WebAssemblyQtVersion::minimumSupportedQtVersion();
    });
}

} // namespace Internal
} // namespace WebAssembly
