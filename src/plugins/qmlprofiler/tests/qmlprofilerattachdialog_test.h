// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlprofiler/qmlprofilerattachdialog.h>
#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerAttachDialogTest : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerAttachDialogTest(QObject *parent = 0);

private slots:
    void testAccessors();
};

} // namespace Internal
} // namespace QmlProfiler
