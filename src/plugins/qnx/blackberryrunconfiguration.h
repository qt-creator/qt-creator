/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BLACKBERRYRUNCONFIGURATION_H
#define QNX_INTERNAL_BLACKBERRYRUNCONFIGURATION_H

#include <projectexplorer/runconfiguration.h>

namespace ProjectExplorer {
class Target;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4Project;
}

namespace Qnx {
namespace Internal {

class BlackBerryDeployConfiguration;

class BlackBerryRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class BlackBerryRunConfigurationFactory;

public:
    explicit BlackBerryRunConfiguration(ProjectExplorer::Target *parent, const Core::Id id, const QString &path);

    QWidget *createConfigurationWidget();

    QString proFilePath() const;

    QString deviceName() const;
    QString barPackage() const;

    QString localExecutableFilePath() const;

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    BlackBerryDeployConfiguration *deployConfiguration() const;
    Qt4ProjectManager::Qt4BuildConfiguration *activeQt4BuildConfiguration() const;

    QString key() const;

signals:
    void targetInformationChanged();

protected:
    BlackBerryRunConfiguration(ProjectExplorer::Target *parent, BlackBerryRunConfiguration *source);

private:
    void init();
    void updateDisplayName();

    QString m_proFilePath;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYRUNCONFIGURATION_H
