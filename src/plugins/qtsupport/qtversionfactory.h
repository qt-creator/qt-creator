// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <projectexplorer/kitaspect.h>

#include <utils/store.h>

#include <QHash>

QT_FORWARD_DECLARE_CLASS(ProKey)
QT_FORWARD_DECLARE_CLASS(ProString)

namespace Utils { class FilePath; }

namespace QtSupport {

class QtVersion;

class QTSUPPORT_EXPORT QtVersionFactory
{
public:
    QtVersionFactory();
    virtual ~QtVersionFactory();

    static const QList<QtVersionFactory *> allQtVersionFactories();

    bool canRestore(const QString &type);
    QtVersion *restore(const QString &type, const Utils::Store &data, const Utils::FilePath &workingDirectory);

    /// factories with higher priority are asked first to identify
    /// a qtversion, the priority of the desktop factory is 0 and
    /// the desktop factory claims to handle all paths
    int priority() const { return m_priority; }

    static QtVersion *createQtVersionFromQMakePath(
        const Utils::FilePath &qmakePath,
        const ProjectExplorer::DetectionSource &detectionSource,
        QString *error = nullptr);

    // For a Qt installation that describes itself in files only, without a runnable
    // qmake or qtpaths, e.g. a cross build unpacked on the target device.
    static QtVersion *createQtVersionFromPrefix(
        const Utils::FilePath &prefix,
        const ProjectExplorer::DetectionSource &detectionSource,
        QString *error = nullptr);

    // Where the description of a Qt version comes from: running a qmake or qtpaths
    // command, or reading the installation's own files.
    enum class QtSource { Command, Files };

protected:
    struct SetupData
    {
        QStringList platforms;
        QStringList config;
        bool isQnx = false; // eeks
        QString mkspec;
    };

    void setQtVersionCreator(const std::function<QtVersion *()> &creator);
    void setRestrictionChecker(const std::function<bool(const SetupData &)> &checker);
    void setSupportedType(const QString &type);
    void setPriority(int priority);

private:
    friend class QtVersion;
    QtVersion *create() const;

    // "qtPath" is what identifies the Qt version, a qmake or qtpaths command, or a
    // prefix when the data was read from the installation's own files.
    static QtVersion *createQtVersion(const Utils::FilePath &qtPath,
                                      const QHash<ProKey, ProString> &versionInfo,
                                      QtSource source,
                                      const ProjectExplorer::DetectionSource &detectionSource,
                                      QString *error);

    std::function<QtVersion *()> m_creator;
    std::function<bool(const SetupData &)> m_restrictionChecker;
    QString m_supportedType;
    int m_priority = 0;
};

} // namespace QtSupport
