// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QProcess>

#include <memory>

namespace QmlDesigner {

class QProcessUniquePointerDeleter
{
public:
    void operator()(QProcess *process)
    {
        process->disconnect();
        QObject::connect(process, &QProcess::finished, process, &QProcess::deleteLater);
        process->kill();
    }
};

using QProcessUniquePointer = std::unique_ptr<QProcess, QProcessUniquePointerDeleter>;

} // namespace QmlDesigner
