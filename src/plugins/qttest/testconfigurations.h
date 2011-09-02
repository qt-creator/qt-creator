/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef TESTCONFIGURATIONS_H
#define TESTCONFIGURATIONS_H

#include <projectexplorer/project.h>
#include <utils/ssh/sshconnection.h>

#include <QPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

class TestConfigurationsPrivate;

class TestConfig : public QObject
{
    Q_OBJECT
public:
    TestConfig();
    virtual ~TestConfig();

    bool load(uint version, QTextStream *s);
    bool save(QTextStream *s);
    void clear();

    bool isActive() { return m_activeProject != 0; }
    QPointer<ProjectExplorer::Project> activeProject() { return m_activeProject; }

    QString configName();
    QString runParams();
    QString srcPath();
    QString buildPath();
    QString currentTestCase();
    QString currentTestFunc();
    QStringList selectedTests();
    QString copyrightHeader();
    QString runScript();
    QString postProcessScript();

    QString makeCommand();
    QString qmakeCommand(bool desktopQMakeRequested);

    QStringList *buildEnvironment();
    QStringList *runEnvironment();

    QString uploadChange();
    void setUploadChange(const QString &newValue);

    QString uploadBranch();
    void setUploadBranch(const QString &newValue);

    QString uploadBranchSpecialization() const;
    void setUploadBranchSpecialization(const QString &newValue);

    QString uploadPlatform();
    void setUploadPlatform(const QString &newValue);

    bool autoDetectPlatformConfiguration() const;
    void setAutoDetectPlatformConfiguration(bool doAutoDetect);

    QString QMAKESPEC();
    void setQMAKESPEC(const QString &newValue);

    QString QMAKESPECSpecialization() const;
    void setQMAKESPECSpecialization(const QString &newValue);

    QString PATH();
    QString QTDIR();

    bool uploadUsingScp();
    bool uploadUsingEMail();
    void setUploadMethod(bool useScp);

    enum UploadMode { UploadAuto, UploadAskMe, UploadNoThanks };

    void setUploadMode(UploadMode mode);
    UploadMode uploadMode();
    bool uploadResults();

    bool isRemoteTarget(QString &deviceName, QString &testDeviceType, Utils::SshConnectionParameters &sshParameters);

    QStringList extraTests() const;
    void setExtraTests(const QStringList &list);

    void setCurrentTest(const QString &testcase, const QString &testFunc);
    void setSelectedTests(const QStringList &list);
    void setConfigName(const QString &name) { m_configName = name; }
    void setRunParams(const QString &params) { m_runParams = params; }
    void setRunScript(const QString &script) { m_runScript = script; }
    void setPostProcessScript(const QString &script) { m_postprocessScript = script; }

    void activate(ProjectExplorer::Project *project);
    void deactivate();

protected:
    void emitConfigChanged();

private slots:
    void onProjectSettingsChanged();
    void onBuildDirectoryChanged();
    void onEnvironmentChanged();
    void onActiveTargetChanged();

private:
    QTimer m_configChangedTimer;
    QStringList m_buildEnvironment;
    QStringList m_runEnvironment;
    QString m_configName;
    QString m_runParams;
    QString m_runScript;
    QString m_postprocessScript;
    QString m_lastTestcase;
    QString m_lastTestfunc;
    QStringList m_lastSelectedTests;
    QString m_uploadChange;
    QString m_uploadBranch;
    QString m_uploadPlatform;
    QString m_uploadBranchSpecialization;
    QString m_qmakeMkspec;
    QString m_qmakeMkspecSpecialization;
    QStringList m_extraTests;
    QPointer<ProjectExplorer::Project> m_activeProject;
    bool m_autodetectPlatformConfiguration;
    QString m_uploadMethod;
    QStringList m_extraTestPaths;
    UploadMode m_uploadMode;

    QString m_currentLoadedSrcDir;
    QStringList m_currentLoadedExtraDirs;
    bool m_changeDetected;
    bool m_branchDetected;
    bool m_platformDetected;
    QString m_makeCommand;
    QString m_qmakeCommand;

private:
    friend class TestConfigurationsPrivate;

    void loadLine(QTextStream *s, const QString &id, QString &value);
    void loadLine(QTextStream *s, const QString &id, int &value);
    void saveLine(QTextStream *s, const QString &id, const QString &value);
};

class TestConfigurations : public QObject
{
    Q_OBJECT
public:
    TestConfigurations();
    ~TestConfigurations();

    static TestConfigurations &instance();

    TestConfig *activeConfiguration();
    void setActiveConfiguration(ProjectExplorer::Project *project);

    TestConfig *config(const QString &configName);
    TestConfig *findConfig(const QString &srcPath);

    void rescan();

    QStringList selectedTests();
    void setSelectedTests(const QStringList &list);

    void setCurrentTest(const QString &testCase, const QString &testFunction);
    QString currentTestCase();
    QString currentTestFunction();

    void delayConfigUpdates(bool delay);
    bool updatesDelayed();

public slots:
    void onActiveConfigurationChanged();
    void onTestSelectionChanged(const QStringList&);

signals:
    void activeConfigurationChanged();
    void testSelectionChanged(const QStringList&, QObject*);

private:
    static TestConfigurations *m_instance;

    TestConfigurationsPrivate *d;
};

#endif
