// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <utils/algorithm.h>
#include <utils/globaltasktree.h>
#include <utils/id.h>
#include <utils/result.h>
#include <utils/shutdownguard.h>

#include <QTcpSocket>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;
using namespace std::string_view_literals;

namespace Lua::Internal {

static SshHostKeyCheckingMode parseHostKeyCheckingMode(
    const QString &mode, SshHostKeyCheckingMode def)
{
    if (mode == "none")
        return SshHostKeyCheckingNone;
    if (mode == "strict")
        return SshHostKeyCheckingStrict;
    if (mode == "allowNoMatch")
        return SshHostKeyCheckingAllowNoMatch;
    return def;
}

void setupDeviceModule()
{
    registerProvider("Device", [](sol::state_view lua) -> sol::object {
        sol::table asyncModule = lua.script("return require('async')", "_device_")
                                     .get<sol::table>();
        sol::function wrap = asyncModule["wrap"];

        sol::table result = lua.create_table();

        // Creates a device of the given type (e.g. "GenericLinuxOsType") from the
        // SSH parameters in `params` and adds it to the DeviceManager. Returns the
        // new device id.
        result["createDevice"] = [](const sol::table &params) -> QString {
            const Id typeId = Id::fromString(params.get<QString>("type"));
            IDeviceFactory *factory = IDeviceFactory::find(typeId);
            if (!factory)
                throw sol::error(QString("Unknown device type: " + typeId.toString()).toStdString());
            if (!factory->canCreate())
                throw sol::error("Device type cannot be created programmatically.");

            IDevice::Ptr device = factory->construct();
            if (!device)
                throw sol::error("Failed to construct device.");
            if (!device->id().isValid())
                device->setupId(IDevice::ManuallyAdded);

            if (auto dn = params.get<std::optional<QString>>("displayName"); dn && !dn->isEmpty())
                device->setDisplayName(*dn);

            SshParameters ssh = device->sshParameters();
            if (auto v = params.get<std::optional<QString>>("host"))
                ssh.setHost(*v);
            if (auto v = params.get<std::optional<int>>("port"))
                ssh.setPort(*v);
            if (auto v = params.get<std::optional<QString>>("userName"))
                ssh.setUserName(*v);
            if (auto v = params.get<std::optional<QString>>("privateKeyFile"))
                ssh.setPrivateKeyFile(FilePath::fromUserInput(*v));
            if (auto v = params.get<std::optional<bool>>("useKeyFile"))
                ssh.setAuthenticationType(*v ? SshParameters::AuthenticationTypeSpecificKey
                                             : SshParameters::AuthenticationTypeAll);
            if (auto v = params.get<std::optional<int>>("timeout"))
                ssh.setTimeout(*v);
            if (auto v = params.get<std::optional<QString>>("hostKeyCheckingMode"))
                ssh.setHostKeyCheckingMode(parseHostKeyCheckingMode(*v, ssh.hostKeyCheckingMode()));

            device->sshParametersAspectContainer().setSshParameters(ssh);
            device->sshParametersAspectContainer().apply();

            DeviceManager::addDevice(device);
            return device->id().toString();
        };

        result["removeDevice"] = [](const QString &id) {
            if (IDevice::ConstPtr d = DeviceManager::find(Id::fromString(id)))
                DeviceManager::removeDevice(d->id());
        };

        // Connects to the device and runs the same auto-detection as the device
        // configuration's "Run Auto-detection": toolchains, debuggers and Qt
        // versions, then (re)creates the device's kits. Awaitable: returns a list
        // of {id, name, valid} kit tables, or an error string on failure.
        result["detectTools_cb"] = [](const QString &id, const sol::main_function &callback,
                                      const sol::this_state &thisState) {
            IDevice::Ptr device = DeviceManager::find(Id::fromString(id));
            if (!device) {
                callback(QString("No such device: %1").arg(id));
                return;
            }

            auto reportKits = [device, callback, thisState]() {
                const bool hasKits = Utils::anyOf(KitManager::kits(), [&](Kit *k) {
                    return BuildDeviceKitAspect::deviceId(k) == device->id();
                });
                if (!hasKits) {
                    KitManager::createKitsForBuildDevice(device);
                } else {
                    for (Kit *k : KitManager::kits()) {
                        if (BuildDeviceKitAspect::deviceId(k) == device->id())
                            KitManager::completeKit(k);
                    }
                }

                sol::state_view lua(thisState);
                sol::table kits = lua.create_table();
                for (Kit *k : KitManager::kits()) {
                    if (BuildDeviceKitAspect::deviceId(k) == device->id()
                        || RunDeviceKitAspect::deviceId(k) == device->id()) {
                        sol::table t = lua.create_table();
                        t["id"] = k->id().toString();
                        t["name"] = k->displayName();
                        t["valid"] = k->isValid();
                        kits.add(t);
                    }
                }
                callback(kits);
            };

            auto onConnected = [device, reportKits, callback](const Result<> &res) {
                if (!res) {
                    callback(res.error());
                    return;
                }
                // Toolchain and debugger detection run synchronously here; on-device
                // tools (cmake, rsync, ...) are detected by the async recipe, after
                // which the kits are (re)created.
                device->requestToolDetection(device->toolSearchPaths());
                GlobalTaskTree::start(device->autoDetectDeviceToolsRecipe(), {}, reportKits);
            };

            device->tryToConnect({Utils::shutdownGuard(), onConnected});
        };
        result["detectTools"] = wrap(result["detectTools_cb"]);

        // Awaitable: true if a TCP connection to host:port succeeds within
        // timeoutMs (default 5000), else false. Useful to wait for a freshly
        // booted device to start accepting ssh.
        result["isReachable_cb"] = [](const QString &host, int port, int timeoutMs,
                                      const sol::main_function &callback) {
            auto *sock = new QTcpSocket;
            auto done = std::make_shared<bool>(false);
            auto finish = [sock, callback, done](bool ok) {
                if (*done)
                    return;
                *done = true;
                callback(ok);
                sock->deleteLater();
            };
            QObject::connect(sock, &QAbstractSocket::connected, sock, [finish] { finish(true); });
            QObject::connect(sock, &QAbstractSocket::errorOccurred, sock, [finish] { finish(false); });
            QTimer::singleShot(timeoutMs > 0 ? timeoutMs : 5000, sock, [finish] { finish(false); });
            sock->connectToHost(host, quint16(port));
        };
        result["isReachable"] = wrap(result["isReachable_cb"]);

        return result;
    });
}

} // namespace Lua::Internal
