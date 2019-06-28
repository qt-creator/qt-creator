/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qbssession.h"

#include <utils/environment.h>

#include <QFutureInterface>
#include <QJsonObject>
#include <QObject>

namespace QbsProjectManager {
namespace Internal {

class QbsBuildSystem;

class QbsProjectParser : public QObject
{
    Q_OBJECT

public:
    QbsProjectParser(QbsBuildSystem *buildSystem, QFutureInterface<bool> *fi);
    ~QbsProjectParser() override;

    void parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir,
               const QString &configName);
    void cancel();
    Utils::Environment environment() const { return m_environment; }

    QbsSession *session() const { return m_session; }
    QJsonObject projectData() const { return m_projectData; }
    ErrorInfo error() const { return m_error; }

signals:
    void done(bool success);

private:
    Utils::Environment m_environment;
    const QString m_projectFilePath;
    QbsSession * const m_session;
    ErrorInfo m_error;
    QJsonObject m_projectData;
    bool m_parsing = false;
    QFutureInterface<bool> *m_fi = nullptr;
};

} // namespace Internal
} // namespace QbsProjectManager
