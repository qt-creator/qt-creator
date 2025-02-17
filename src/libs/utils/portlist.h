// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "commandline.h"
#include "port.h"

namespace Tasking {
class ExecutableItem;
template <typename StorageStruct>
class Storage;
}

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Utils {
namespace Internal { class PortListPrivate; }

class QTCREATOR_UTILS_EXPORT PortList
{
public:
    PortList();
    PortList(const PortList &other);
    PortList &operator=(const PortList &other);
    ~PortList();

    void addPort(Port port);
    void addRange(Port startPort, Port endPort);
    bool hasMore() const;
    bool contains(Port port) const;
    int count() const;
    Port getNext();
    Port getNextFreePort(const QList<Port> &usedPorts);
    QString toString() const;

    static PortList fromString(const QString &portsSpec);
    static QString regularExpression();

private:
    Internal::PortListPrivate * const d;
};

class QTCREATOR_UTILS_EXPORT PortsInputData
{
public:
    PortList freePorts;
    CommandLine commandLine;
    std::function<QList<Port>(const QByteArray &)> portsParser = &Port::parseFromCommandOutput;
};

using PortsOutputData = expected_str<QList<Port>>;

QTCREATOR_UTILS_EXPORT Tasking::ExecutableItem portsFromProcessRecipe(
    const Tasking::Storage<PortsInputData> &input, const Tasking::Storage<PortsOutputData> &output);

} // namespace Utils
