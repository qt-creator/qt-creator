// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Python::Internal {

class PythonSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testDropsADefaultThatIsGone();
};

QObject *createPythonSettingsTest();

} // namespace Python::Internal
