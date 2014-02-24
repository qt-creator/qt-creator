/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef VCPROJECTMANAGER_INTERNAL_MSBUILDVERSIONMANAGER_H
#define VCPROJECTMANAGER_INTERNAL_MSBUILDVERSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QList>

#include <coreplugin/id.h>

namespace VcProjectManager {
namespace Internal {

class MsBuildInformation {
public:
    enum MsBuildVersion {
        MSBUILD_V_2_0 = 0,
        MSBUILD_V_3_0 = 1,
        MSBUILD_V_3_5 = 2,
        MSBUILD_V_4_0 = 3,
        MSBUILD_V_UNKNOWN = 4
    };

    MsBuildInformation(const QString &executablePath, const QString &version)
        : m_executable(executablePath),
          m_versionString(version)
    {
        if (version.startsWith(QLatin1String("2.")))
            m_msBuildVersion = MSBUILD_V_2_0;
        else if (version.startsWith(QLatin1String("3.0")))
            m_msBuildVersion = MSBUILD_V_3_0;
        else if (version.startsWith(QLatin1String("3.5")))
            m_msBuildVersion = MSBUILD_V_3_5;
        else if (version.startsWith(QLatin1String("4.0")))
            m_msBuildVersion = MSBUILD_V_4_0;
        else
            m_msBuildVersion = MSBUILD_V_UNKNOWN;

        QString temp = executablePath;
        temp += version;
        m_id = Core::Id(temp.toStdString().c_str());
    }

    Core::Id getId() const
    {
        return m_id;
    }

    void setId(Core::Id id)
    {
        m_id = id;
    }

    QString m_executable;
    MsBuildVersion m_msBuildVersion;
    QString m_versionString;

private:
    Core::Id m_id;
};

class MsBuildVersionManager : public QObject
{
    friend class VcProjectManagerPlugin;

    Q_OBJECT

public:
    static MsBuildVersionManager *instance();
    ~MsBuildVersionManager();

    bool addMsBuildInformation(MsBuildInformation *msBuildInfo);
    QList<MsBuildInformation *> msBuildInformations() const;
    MsBuildInformation *msBuildInformation(Core::Id msBuildID);
    MsBuildInformation *msBuildInformation(MsBuildInformation::MsBuildVersion minVersion, MsBuildInformation::MsBuildVersion maxVersion);
    void removeMsBuildInformation(const Core::Id &msBuildId);
    void replace(Core::Id targetMsBuild, MsBuildInformation *newMsBuild);

    void saveSettings();

    static MsBuildInformation *createMsBuildInfo(const QString &executablePath, const QString &version);

signals:
    void msBuildAdded(Core::Id id);
    void msBuildReplaced(Core::Id oldMsBuild, Core::Id newMsBuild);
    void msBuildRemoved(Core::Id msBuildId);

private:
    MsBuildVersionManager();
    void loadSettings();

    static MsBuildVersionManager *m_instance;
    QList<MsBuildInformation *> m_msBuildInfos;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_MSBUILDVERSIONMANAGER_H
