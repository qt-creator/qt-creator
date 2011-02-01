/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMOMANAGER_H
#define MAEMOMANAGER_H

#include <QtCore/QObject>

namespace Qt4ProjectManager {
    class QtVersion;
namespace Internal {

class MaemoDeployStepFactory;
class MaemoPackageCreationFactory;
class MaemoPublishingWizardFactoryFremantleFree;
class MaemoRunControlFactory;
class MaemoRunConfigurationFactory;
class MaemoDeviceConfigurationsSettingsPage;
class MaemoQemuManager;
class MaemoQemuSettingsPage;
class Qt4MaemoTargetFactory;
class MaemoToolChainFactory;

class MaemoManager : public QObject
{
    Q_OBJECT

public:
    MaemoManager();
    ~MaemoManager();
    static MaemoManager &instance();

    MaemoDeviceConfigurationsSettingsPage *deviceConfigurationsSettingsPage() const { return m_deviceConfigurationsSettingsPage; }
    MaemoQemuSettingsPage *qemuSettingsPage() const { return m_qemuSettingsPage; }

private:
    static MaemoManager *m_instance;

    MaemoRunControlFactory *m_runControlFactory;
    MaemoRunConfigurationFactory *m_runConfigurationFactory;
    MaemoPackageCreationFactory *m_packageCreationFactory;
    MaemoDeployStepFactory *m_deployStepFactory;
    MaemoDeviceConfigurationsSettingsPage *m_deviceConfigurationsSettingsPage;
    MaemoQemuSettingsPage *m_qemuSettingsPage;
    MaemoQemuManager *m_qemuRuntimeManager;
    MaemoPublishingWizardFactoryFremantleFree *m_publishingFactoryFremantleFree;
    Qt4MaemoTargetFactory *m_maemoTargetFactory;
    MaemoToolChainFactory *m_toolChainFactory;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOMANAGER_H
