// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionfactory.h>

#include <QCoreApplication>

namespace Android {
namespace Internal {

class AndroidQtVersion : public QtSupport::QtVersion
{
public:
    AndroidQtVersion();

    bool isValid() const override;
    QString invalidReason() const override;

    bool supportsMultipleQtAbis() const override;
    ProjectExplorer::Abis detectQtAbis() const override;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    void setupQmakeRunEnvironment(Utils::Environment &env) const override;

    QSet<Utils::Id> availableFeatures() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;

    QString description() const override;
    const QStringList &androidAbis() const;
    int minimumNDK() const;

    static QString androidDeploymentSettingsFileName(const ProjectExplorer::Target *target);
    static Utils::FilePath androidDeploymentSettings(const ProjectExplorer::Target *target);

    struct BuiltWith {
        int apiVersion = -1;
        QVersionNumber ndkVersion;
        QString androidAbi;
    };
    static BuiltWith parseBuiltWith(const QByteArray &modulesCoreJsonData, bool *ok = nullptr);
    BuiltWith builtWith(bool *ok = nullptr) const;

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
