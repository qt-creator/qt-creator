// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakestep.h"

namespace QmakeProjectManager {

class QmakeExtraBuildInfo final
{
public:
    QmakeExtraBuildInfo();

    QString additionalArguments;
    QString makefile;
    QMakeStepConfig config;
};

} // namespace QmakeProjectManager

Q_DECLARE_METATYPE(QmakeProjectManager::QmakeExtraBuildInfo)
