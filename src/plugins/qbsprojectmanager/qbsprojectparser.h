// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qbssession.h"

#include <utils/environment.h>

#include <QFutureInterface>
#include <QJsonObject>
#include <QObject>

namespace QbsProjectManager::Internal {

class QbsBuildSystem;

class QbsProjectParser : public QObject
{
    Q_OBJECT

public:
    QbsProjectParser(QbsBuildSystem *buildSystem, QFutureInterface<bool> *fi);
    ~QbsProjectParser() override;

    void parse(const QVariantMap &config,
               const Utils::Environment &env,
               const Utils::FilePath &dir,
               const QString &configName);
    void cancel();
    Utils::Environment environment() const { return m_environment; }

    // FIXME: Why on earth do we not own the FutureInterface?
    void deleteLaterSafely() { m_fi = nullptr; deleteLater(); }

    QbsSession *session() const { return m_session; }
    QJsonObject projectData() const { return m_projectData; }
    ErrorInfo error() const { return m_error; }

signals:
    void done(bool success);

private:
    Utils::Environment m_environment;
    const Utils::FilePath m_projectFilePath;
    QbsSession * const m_session;
    ErrorInfo m_error;
    QJsonObject m_projectData;
    bool m_parsing = false;
    QFutureInterface<bool> *m_fi = nullptr;
};

} // QbsProjectManager::Internal
