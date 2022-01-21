/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "webassemblyconstants.h"
#include "webassemblyqtversion.h"

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
#include <QFileInfo>
#include <QVersionNumber>

using namespace QtSupport;
using namespace Utils;

namespace WebAssembly {
namespace Internal {

WebAssemblyQtVersion::WebAssemblyQtVersion() = default;

QString WebAssemblyQtVersion::description() const
{
    return QCoreApplication::translate("WebAssemblyPlugin", "WebAssembly",
                                       "Qt Version is meant for WebAssembly");
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

    return tr("%1 does not support Qt for WebAssembly below version %2.")
            .arg(Core::ICore::versionString())
            .arg(QVersionNumber(minimumSupportedQtVersion().majorVersion,
                                minimumSupportedQtVersion().minorVersion).toString());
}

const QtVersionNumber &WebAssemblyQtVersion::minimumSupportedQtVersion()
{
    const static QtVersionNumber number(5, 15);
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
