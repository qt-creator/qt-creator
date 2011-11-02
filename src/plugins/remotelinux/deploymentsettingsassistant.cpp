/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include "deploymentsettingsassistant.h"

#include "deploymentinfo.h"
#include "deployablefile.h"
#include "deployablefilesperprofile.h"
#include "profilesupdatedialog.h"

#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <qt4projectmanager/qt4nodes.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtGui/QMainWindow>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
namespace {

enum ProFileUpdateSetting { UpdateProFile, DontUpdateProFile };
typedef QHash<QString, ProFileUpdateSetting> UpdateSettingsMap;

} // anonymous namespace

class DeploymentSettingsAssistantInternal
{
public:
    DeploymentSettingsAssistantInternal(const QString &qmakeScope, const QString &installPrefix,
            const QSharedPointer<DeploymentInfo> &deploymentInfo)
        : qmakeScope(qmakeScope), installPrefix(installPrefix), deploymentInfo(deploymentInfo)
    {
    }

    const QString qmakeScope;
    const QString installPrefix;
    const QSharedPointer<DeploymentInfo> deploymentInfo;
    UpdateSettingsMap updateSettings;
};

} // namespace Internal

using namespace Internal;

DeploymentSettingsAssistant::DeploymentSettingsAssistant(const QString &qmakeScope,
        const QString &installPrefix, const QSharedPointer<DeploymentInfo> &deploymentInfo,
        QObject *parent)
    : QObject(parent),
      d(new DeploymentSettingsAssistantInternal(qmakeScope, installPrefix, deploymentInfo))
{
    connect(d->deploymentInfo.data(), SIGNAL(modelReset()), SLOT(handleDeploymentInfoUpdated()));
}

DeploymentSettingsAssistant::~DeploymentSettingsAssistant()
{
    delete d;
}

bool DeploymentSettingsAssistant::addDeployableToProFile(const DeployableFilesPerProFile *proFileInfo,
    const QString &variableName, const DeployableFile &deployable)
{
    const QString filesLine = variableName + QLatin1String(".files = ")
        + QDir(proFileInfo->projectDir()).relativeFilePath(deployable.localFilePath);
    const QString pathLine = variableName + QLatin1String(".path = ") + deployable.remoteDir;
    const QString installsLine = QLatin1String("INSTALLS += ") + variableName;
    return addLinesToProFile(proFileInfo, QStringList() << filesLine << pathLine << installsLine);
}

bool DeploymentSettingsAssistant::addLinesToProFile(const DeployableFilesPerProFile *proFileInfo,
    const QStringList &lines)
{
    Core::FileChangeBlocker update(proFileInfo->proFilePath());

    const QString separator = QLatin1String("\n    ");
    const QString proFileString = QLatin1Char('\n') + d->qmakeScope + QLatin1String(" {")
        + separator + lines.join(separator) + QLatin1String("\n}\n");
    Utils::FileSaver saver(proFileInfo->proFilePath(), QIODevice::Append);
    saver.write(proFileString.toLocal8Bit());
    return saver.finalize(Core::ICore::instance()->mainWindow());
}

void DeploymentSettingsAssistant::handleDeploymentInfoUpdated()
{
    QList<DeployableFilesPerProFile *> proFilesToAskAbout;
    QList<DeployableFilesPerProFile *> proFilesToUpdate;
    for (int i = 0; i < d->deploymentInfo->modelCount(); ++i) {
        DeployableFilesPerProFile * const proFileInfo = d->deploymentInfo->modelAt(i);
        if (proFileInfo->projectType() != AuxTemplate && !proFileInfo->hasTargetPath()) {
            const UpdateSettingsMap::ConstIterator it
                = d->updateSettings.find(proFileInfo->proFilePath());
            if (it == d->updateSettings.constEnd())
                proFilesToAskAbout << proFileInfo;
            else if (it.value() == UpdateProFile)
                proFilesToUpdate << proFileInfo;
        }
    }

    if (!proFilesToAskAbout.isEmpty()) {
        ProFilesUpdateDialog dialog(proFilesToAskAbout);
        dialog.exec();
        const QList<ProFilesUpdateDialog::UpdateSetting> &settings = dialog.getUpdateSettings();
        foreach (const ProFilesUpdateDialog::UpdateSetting &setting, settings) {
            const ProFileUpdateSetting updateSetting = setting.second
                ? UpdateProFile : DontUpdateProFile;
            d->updateSettings.insert(setting.first->proFilePath(), updateSetting);
            if (updateSetting == UpdateProFile)
                proFilesToUpdate << setting.first;
        }
    }

    foreach (const DeployableFilesPerProFile * const proFileInfo, proFilesToUpdate) {
        const QString remoteDirSuffix = QLatin1String(proFileInfo->projectType() == LibraryTemplate
                ? "/lib" : "/bin");
        const QString remoteDir = QLatin1String("target.path = ") + d->installPrefix
            + QLatin1Char('/') + proFileInfo->projectName() + remoteDirSuffix;
        const QStringList deployInfo = QStringList() << remoteDir
            << QLatin1String("INSTALLS += target");
        addLinesToProFile(proFileInfo, deployInfo);
    }
}

} // namespace RemoteLinux
