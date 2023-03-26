// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include "iostooltypes.h"

namespace Ios {
class IosTool;
class GdbRunner: public QObject
{
    Q_OBJECT

public:
    GdbRunner(IosTool *iosTool, ServiceConnRef conn);
    void stop(int phase);
    void run();

signals:
    void finished();

private:
    IosTool *m_iosTool;
    ServiceConnRef m_conn = nullptr;
};
}
