// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlbase.h"
#include "qmlconfiguration.h"

class QmlRuntime : public QmlBase
{
    using QmlBase::QmlBase;

private:
    void initCoreApp() override;
    void populateParser() override;
    void initQmlRunner() override;

    void listConfFiles();
    void loadConf(const QString &override, bool quiet);

    const QString m_iconResourcePath = QStringLiteral(":/qt-project.org/QmlRuntime/resources/qml-64.png");
    const QString m_confResourcePath = QStringLiteral(":/runner/runnerconf/qmlruntime/");


    QSharedPointer<Config> m_conf;
    bool m_verboseMode = false;
    bool m_quietMode = false;
    int m_exitTimerId = -1;
};

