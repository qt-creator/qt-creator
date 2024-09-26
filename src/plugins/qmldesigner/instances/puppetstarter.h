// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qprocessuniqueptr.h"

#include <puppetstartdata.h>

#include <utils/id.h>

#include <QString>
#include <QProcessEnvironment>

#include <designersettings.h>

namespace QmlDesigner::PuppetStarter {

QProcessUniquePointer createPuppetProcess(const PuppetStartData &data,
                                          const QString &puppetMode,
                                          const QString &socketToken,
                                          std::function<void()> processOutputCallback,
                                          std::function<void(int, QProcess::ExitStatus)> processFinishCallback,
                                          const QStringList &customOptions = {});

} // namespace QmlDesigner::PuppetStarter
