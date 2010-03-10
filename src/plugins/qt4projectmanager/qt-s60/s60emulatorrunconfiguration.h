/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef S60EMULATORRUNCONFIGURATION_H
#define S60EMULATORRUNCONFIGURATION_H

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>

#include <QtCore/QVariantMap>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
}

namespace Qt4ProjectManager {
class Qt4Project;

namespace Internal {
class Qt4ProFileNode;
class Qt4Target;
class S60EmulatorRunConfigurationFactory;

class S60EmulatorRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class S60EmulatorRunConfigurationFactory;

public:
    S60EmulatorRunConfiguration(ProjectExplorer::Target *parent, const QString &proFilePath);
    virtual ~S60EmulatorRunConfiguration();

    Qt4Target *qt4Target() const;

    bool isEnabled(ProjectExplorer::BuildConfiguration *configuration) const;
    QWidget *configurationWidget();

    QString executable() const;

    QVariantMap toMap() const;

signals:
    void targetInformationChanged();

private slots:
    void proFileUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *pro);

protected:
    S60EmulatorRunConfiguration(ProjectExplorer::Target *parent, S60EmulatorRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();
    void updateTarget();

    QString m_proFilePath;
};

class S60EmulatorRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    S60EmulatorRunConfigurationWidget(S60EmulatorRunConfiguration *runConfiguration,
                                      QWidget *parent = 0);

private slots:
    void displayNameEdited(const QString &text);
    void updateTargetInformation();

private:
    S60EmulatorRunConfiguration *m_runConfiguration;
    Utils::DetailsWidget *m_detailsWidget;
    QLineEdit *m_nameLineEdit;
    QLabel *m_executableLabel;
};

class S60EmulatorRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    explicit S60EmulatorRunConfigurationFactory(QObject *parent = 0);
    ~S60EmulatorRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *project, const QString &id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QStringList availableCreationIds(ProjectExplorer::Target *pro) const;
    // used to translate the ids to names to display to the user
    QString displayNameForId(const QString &id) const;
};

class S60EmulatorRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit S60EmulatorRunControl(S60EmulatorRunConfiguration *runConfiguration);
    ~S60EmulatorRunControl() {}
    void start();
    void stop();
    bool isRunning() const;

private slots:
    void processExited(int exitCode);
    void slotAddToOutputWindow(const QString &line);
    void slotError(const QString & error);

private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QString m_executable;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60EMULATORRUNCONFIGURATION_H
