/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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
**
**************************************************************************/

#ifndef S60DEVICERUNCONFIGURATION_H
#define S60DEVICERUNCONFIGURATION_H

#include "../qmakerunconfigurationfactory.h"
#include "../qt4projectmanager_global.h"

#include <projectexplorer/runconfiguration.h>

#include <QFutureInterface>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace ProjectExplorer { class Node; }

namespace Qt4ProjectManager {
class Qt4ProFileNode;

namespace Internal { class SymbianQtVersion; }

class S60DeviceRunConfigurationFactory;

class QT4PROJECTMANAGER_EXPORT S60DeviceRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class S60DeviceRunConfigurationFactory;

public:
    S60DeviceRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    virtual ~S60DeviceRunConfiguration();

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();

    Utils::OutputFormatter *createOutputFormatter() const;

    QString commandLineArguments() const;
    void setCommandLineArguments(const QString &args);
    QString qmlCommandLineArguments() const;

    QString projectFilePath() const;

    QString targetName() const;
    QString localExecutableFileName() const;
    quint32 executableUid() const;

    bool isDebug() const;
    QString symbianTarget() const;

    QVariantMap toMap() const;

    QString proFilePath() const;
signals:
    void targetInformationChanged();

protected:
    S60DeviceRunConfiguration(ProjectExplorer::Target *parent, S60DeviceRunConfiguration *source);
    QString defaultDisplayName() const;
    virtual bool fromMap(const QVariantMap &map);

private slots:
    void proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress);

private:
    void ctor();

    QString m_proFilePath;
    QString m_commandLineArguments;
    bool m_validParse;
    bool m_parseInProgress;
};

class S60DeviceRunConfigurationFactory : public QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit S60DeviceRunConfigurationFactory(QObject *parent = 0);
    ~S60DeviceRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    // used to translate the ids to names to display to the user
    QString displayNameForId(const Core::Id id) const;

    bool canHandle(ProjectExplorer::Target *t) const;
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        ProjectExplorer::Node *n);
};

} // namespace Qt4ProjectManager

#endif // S60DEVICERUNCONFIGURATION_H
