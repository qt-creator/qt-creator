/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionfactory.h>

#include <QCoreApplication>

namespace Android {
namespace Internal {

class AndroidQtVersion : public QtSupport::BaseQtVersion
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AndroidQtVersion)

public:
    AndroidQtVersion();

    bool isValid() const override;
    QString invalidReason() const override;

    ProjectExplorer::Abis detectQtAbis() const override;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    Utils::Environment qmakeRunEnvironment() const override;

    QSet<Utils::Id> availableFeatures() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;

    QString description() const override;
    const QStringList &androidAbis() const;
    int minimumNDK() const;

protected:
    void parseMkSpec(ProFileEvaluator *) const override;
private:
    std::unique_ptr<QObject> m_guard;
    mutable QStringList m_androidAbis;
    mutable int m_minNdk = -1;
};

class AndroidQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    AndroidQtVersionFactory();
};

} // namespace Internal
} // namespace Android
