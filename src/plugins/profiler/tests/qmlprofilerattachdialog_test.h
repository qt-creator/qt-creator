// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <profiler/qmlprofilerattachdialog.h>

#include <QObject>

namespace Profiler::Internal {

class QmlProfilerAttachDialogTest : public QObject
{
    Q_OBJECT

public:
    QmlProfilerAttachDialogTest();

private slots:
    void testAccessors();
};

} // namespace Profiler::Internal
