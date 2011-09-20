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

#include "testconfigurations.h"
#include "qsystem.h"
#include "testsettings.h"
#include "testcode.h"
#include "testcontextmenu.h"

#include <coreplugin/imode.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/target.h>
#include <remotelinux/linuxdeviceconfigurations.h>
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <qt4projectmanager/qt-desktop/qt4runconfiguration.h>
#include <qt4projectmanager/qt4buildconfiguration.h>

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QRegExp>
#include <QStringList>
#include <QDebug>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

//*************************************************

TestConfigurations *TestConfigurations::m_instance = 0;

class TestConfigurationsPrivate : public QObject
{
    Q_OBJECT
public:
    TestConfigurationsPrivate();
    ~TestConfigurationsPrivate();

    QList<TestConfig*> m_configList;

    void clear();
    bool load();
    bool save();
    void rescan();

    TestConfig *activeConfiguration();
    void setActiveConfiguration(ProjectExplorer::Project *project);

    TestConfig *config(const QString &cfgName);
    TestConfig *findConfig(const QString &srcPath);

    QStringList selectedTests();
    void setSelectedTests(const QStringList &list);

    void setCurrentTest(const QString &testCase, const QString &testFunction);
    QString currentTestCase();
    QString currentTestFunction();

    void delayConfigUpdates(bool delay);
    bool updatesDelayed() { return m_delayConfigUpdates; }

public slots:
    void onActiveConfigurationChanged();
    void onTestSelectionChanged(const QStringList&, QObject*);

protected:
    bool load(const QString &fileName);
    bool save(const QString &fileName);

signals:
    void activeConfigurationChanged();
    void testSelectionChanged(const QStringList&, QObject*);

private:
    TestCollection m_testCollection;
    bool m_delayConfigUpdates;
};

#include "testconfigurations.moc"

TestConfigurationsPrivate::TestConfigurationsPrivate()
{
    load();
    m_delayConfigUpdates = false;
}

TestConfigurationsPrivate::~TestConfigurationsPrivate()
{
    save();
    clear();
}

void TestConfigurationsPrivate::clear()
{
    while (!m_configList.isEmpty())
        delete m_configList.takeFirst();
}

bool TestConfigurationsPrivate::load()
{
    return load(QDir::homePath() + QDir::separator() + QLatin1String(".qttest")
        + QDir::separator() + QLatin1String("saved_configurations"));
}

bool TestConfigurationsPrivate::load(const QString &fileName)
{
    clear();
    int version = 0;

    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream S(&f);
        QString tmpName = S.readLine();
        if (tmpName.startsWith(QLatin1String("VERSION="))) {
            bool ok;
            tmpName = tmpName.mid(8).simplified();
            version = tmpName.toUInt(&ok);
            if (!ok) {
                qWarning("Could not read version in configurations file");
                return false;
            }
        } else {
            qWarning("Could not read configurations file");
            return false;
        }

        TestConfig *tmp;
        int count = -1;
        while (!S.atEnd()) {
            ++count;
            tmp = new TestConfig;
            if (tmp->load(version, &S))
                m_configList.append(tmp);
            else
                delete tmp;
        }
        return true;
    }
    return false;
}


bool TestConfigurationsPrivate::save()
{
    QDir().mkpath(QDir::homePath() + QDir::separator() + QLatin1String(".qttest"));
    return save(QDir::homePath() + QDir::separator() + QLatin1String(".qttest")
        + QDir::separator() + QLatin1String("saved_configurations"));
}

bool TestConfigurationsPrivate::save(const QString &fileName)
{
    uint curVersion = 9;
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly)) {
        QTextStream S(&f);
        S << QString::fromLatin1("VERSION=%1").arg(curVersion) << '\n';
        TestConfig *tmp;
        for (int i = 0; i < m_configList.count(); ++i) {
            tmp = m_configList.at(i);
            if (tmp)
                tmp->save(&S);
        }
        return true;
    }
    return false;
}

TestConfig *TestConfigurationsPrivate::activeConfiguration()
{
    TestConfig *cfg = 0;
    for (int i = 0; i < m_configList.count(); ++i) {
        cfg = m_configList[i];
        if (cfg && cfg->isActive())
            return cfg;
    }
    return 0;
}

void TestConfigurationsPrivate::setActiveConfiguration(ProjectExplorer::Project *project)
{
    TestCollection tc;
    tc.removeAll();

    TestConfig *cfg = 0;
    for (int i = 0; i < m_configList.count(); ++i) {
        cfg = m_configList[i];
        if (cfg && cfg->isActive())
            cfg->deactivate();
    }

    // search for configuration, and create if it doesn't exist yet.
    if (project) {
        TestConfig *tmp = config(project->displayName());
        if (tmp)
            tmp->activate(project);
    }

    TestContextMenu menu(0);
    menu.updateActions(activeConfiguration() != 0, false, false);

    emit activeConfigurationChanged();
}

TestConfig *TestConfigurationsPrivate::config(const QString &cfgName)
{
    TestConfig *cfg = 0;
    for (int i = 0; i < m_configList.count(); ++i) {
        cfg = m_configList[i];
        if (cfg && cfg->configName() == cfgName)
            return cfg;
    }

    cfg = new TestConfig();
    cfg->setConfigName(cfgName);
    cfg->setRunParams(QString());
    cfg->setRunScript(QString());
    cfg->setPostProcessScript(QString());
    m_configList.append(cfg);

    return cfg;
}

TestConfig *TestConfigurationsPrivate::findConfig(const QString &srcPath)
{
    for (int i = 0; i < m_configList.count(); ++i) {
        TestConfig *tmp = m_configList.at(i);
        if (tmp) {
            QString tmpPath = tmp->srcPath();
            if (!tmpPath.isEmpty() && srcPath.startsWith(tmpPath))
                return tmp;
        }
    }

    for (int i = 0; i < m_configList.count(); ++i) {
        TestConfig *tmp = m_configList.at(i);
        if (tmp) {
            QStringList tmpPaths = tmp->extraTests();
            foreach (const QString &tmpPath, tmpPaths) {
                if (!tmpPath.isEmpty() && srcPath.startsWith(tmpPath))
                    return tmp;
            }
        }
    }

    return 0;
}

QStringList TestConfigurationsPrivate::selectedTests()
{
    QStringList ret;
    for (int i = 0; i < m_configList.count(); ++i) {
        TestConfig *tmp = m_configList.at(i);
        if (tmp && tmp->isActive())
            ret += tmp->selectedTests();
    }
    return ret;
}

void TestConfigurationsPrivate::setSelectedTests(const QStringList &list)
{
    // split up the list into lists *per* test configuration
    // then save each smaller list into it's specific configuration
    for (int i = 0; i < m_configList.count(); ++i) {
        TestConfig *tmp = m_configList.at(i);
        if (tmp && tmp->isActive()) {
            QStringList srcPaths = QStringList(tmp->srcPath());
            if (tmp->extraTests().count() > 0)
                srcPaths.append(tmp->extraTests());

            QStringList tmpList;
            foreach (const QString &selection, list) {
                QString tcname = selection.left(selection.indexOf(QLatin1String("::")));
                TestCode *tc = m_testCollection.findCodeByTestCaseName(tcname);
                if (tc) {
                    foreach (const QString &srcPath, srcPaths) {
                        if (tc->actualFileName().startsWith(srcPath)) {
                            tmpList += selection;
                            break;
                        }
                    }
                }
            }
            tmp->setSelectedTests(tmpList);
        }
    }

    // FIXME This needs to be done more intelligently
    save();
}

void TestConfigurationsPrivate::setCurrentTest(const QString &testCase, const QString &testFunction)
{
    TestConfig *tmp = activeConfiguration();
    if (tmp)
        tmp->setCurrentTest(testCase, testFunction);
}

QString TestConfigurationsPrivate::currentTestCase()
{
    TestConfig *tmp = activeConfiguration();
    if (tmp)
        return tmp->currentTestCase();
    return QString();
}

QString TestConfigurationsPrivate::currentTestFunction()
{
    for (int i = 0; i < m_configList.count(); ++i) {
        TestConfig *tmp = m_configList.at(i);
        if (tmp && tmp->isActive())
            return tmp->currentTestFunc();
    }
    return QString();
}

void TestConfigurationsPrivate::rescan()
{
    m_testCollection.removeAll();

    for (int i = 0; i < m_configList.count(); ++i) {
        TestConfig *tmp = m_configList.at(i);
        if (tmp && tmp->isActive())
            tmp->onProjectSettingsChanged();
    }
}

void TestConfigurationsPrivate::delayConfigUpdates(bool delay)
{
    m_delayConfigUpdates = delay;
}

void TestConfigurationsPrivate::onActiveConfigurationChanged()
{
    emit activeConfigurationChanged();
}

void TestConfigurationsPrivate::onTestSelectionChanged(const QStringList& selection, QObject *originator)
{
    setSelectedTests(selection);
    emit testSelectionChanged(selection, originator);
}

// ************************************************

TestConfig::TestConfig()
{
    m_activeProject = 0;
    m_currentLoadedSrcDir.clear();
    m_currentLoadedExtraDirs.clear();
    clear();
    connect(&m_configChangedTimer, SIGNAL(timeout()), this, SLOT(onProjectSettingsChanged()));
}

TestConfig::~TestConfig()
{
}

void TestConfig::clear()
{
    m_runParams.clear();
    m_runScript.clear();
    m_postprocessScript.clear();
    m_lastTestcase.clear();
    m_lastTestfunc.clear();
    m_uploadChange.clear();
    m_uploadBranch.clear();
    m_uploadBranchSpecialization.clear();
    m_uploadPlatform.clear();
    m_qmakeMkspec.clear();
    m_qmakeMkspecSpecialization.clear();

    m_lastSelectedTests = QStringList();
    m_changeDetected = false;
    m_branchDetected = false;
    m_autodetectPlatformConfiguration = true;
    m_platformDetected = false;
    m_uploadMethod = QLatin1String("SCP");
    m_buildEnvironment.clear();
    m_uploadMode = UploadAuto;
    m_makeCommand.clear();
    m_qmakeCommand.clear();
    m_extraTests.clear();
}

QString TestConfig::runScript()
{
    QString ret = m_runScript;
    QSystem::processEnvValue(buildEnvironment(), ret);
    return ret;
}

QString TestConfig::copyrightHeader()
{
    QString ret = srcPath() + QDir::separator() + QLatin1String("dist") + QDir::separator()
        + QLatin1String("header-dual-license.txt");
    QFileInfo inf(ret);
    if (inf.exists())
        return ret;
    return QString();
}

void TestConfig::setQMAKESPEC(const QString &newValue)
{
    m_qmakeMkspec = newValue;
}

QString TestConfig::QMAKESPEC()
{
    if (m_autodetectPlatformConfiguration && m_activeProject){
        // seek to determine the mkspec from the current Qt target
        Qt4BuildConfiguration *qtBuildConfig =
            static_cast<Qt4BuildConfiguration *>(m_activeProject->activeTarget()->activeBuildConfiguration());

        if (qtBuildConfig)
            return qtBuildConfig->qtVersion()->mkspec();
        else
            return QSystem::envKey(buildEnvironment(), QLatin1String("QMAKESPEC"));
    }

    return m_qmakeMkspec;
}

QString TestConfig::QMAKESPECSpecialization() const
{
    return m_qmakeMkspecSpecialization;
}

void TestConfig::setQMAKESPECSpecialization(const QString &newValue)
{
    m_qmakeMkspecSpecialization = newValue;
}

QString TestConfig::PATH()
{
    return QSystem::envKey(buildEnvironment(), QLatin1String("PATH"));
}

QString TestConfig::QTDIR()
{
    return QSystem::envKey(buildEnvironment(), QLatin1String("QTDIR"));
}

QString TestConfig::runParams()
{
    QString ret = m_runParams;
    QSystem::processEnvValue(buildEnvironment(), ret);
    return ret;
}

QString TestConfig::postProcessScript()
{
    QString ret = m_postprocessScript;
    QSystem::processEnvValue(buildEnvironment(), ret);
    return ret;
}

QString TestConfig::makeCommand()
{
    if (m_makeCommand.isEmpty()) {
        if (QMAKESPEC().contains(QLatin1String("msvc")))
            m_makeCommand = QLatin1String("nmake");
        else if (QMAKESPEC().contains(QLatin1String("mingw")))
            m_makeCommand = QLatin1String("mingw32-make");
        else
            m_makeCommand = QLatin1String("make");
        m_makeCommand = QSystem::which(PATH(), m_makeCommand);
    }
    return m_makeCommand;
}

QString TestConfig::qmakeCommand(bool /*desktopQMakeRequested*/)
{

    if (m_activeProject){
        // seek to determine the qmake command from the current Qt target
        Qt4BuildConfiguration *qtBuildConfig =
            static_cast<Qt4BuildConfiguration *>(m_activeProject->activeTarget()->activeBuildConfiguration());
        return qtBuildConfig->qtVersion()->qmakeCommand();
    }

    qWarning() << "Unable to detect qmake command, falling back to \"qmake\"";
    return QLatin1String("qmake");
}

void TestConfig::setUploadMode(UploadMode mode)
{
    m_uploadMode = mode;
}

TestConfig::UploadMode TestConfig::uploadMode()
{
    return m_uploadMode;
}

bool TestConfig::uploadResults()
{
    if (m_uploadMode == TestConfig::UploadAuto)
        return true;
    if (m_uploadMode == TestConfig::UploadNoThanks)
        return false;
    if (QMessageBox::question(0, tr("Upload Test Results"),
        tr("You can positively influence the quality of %1"
           " by contributing test results.\n\nWould you like to upload your Test Results").arg(configName()),
        QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        return true;
    return false;
}

bool TestConfig::isRemoteTarget(QString &deviceName, QString &testDeviceType,
    Utils::SshConnectionParameters &sshParameters)
{
    if (m_activeProject && m_activeProject->activeTarget()) {
        ProjectExplorer::RunConfiguration *r = m_activeProject->activeTarget()->activeRunConfiguration();
        if (r) {
            if (r->id() == QLatin1String("RemoteLinuxRunConfiguration")) {
                RemoteLinux::RemoteLinuxRunConfiguration *mr = static_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(r);
                if (mr) {
                    QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> mc = mr->deviceConfig();
                    if (mc){
                        deviceName = mc->name();
                        sshParameters = mc->sshParameters();
                        testDeviceType = QLatin1String("RemoteLinux");//Qt4Test::TestController::Maemo;
                        return true;
                    }else{
                        qWarning() << "Invalid remote" << deviceName;
                    }
                }
            } else if (r->id() == QLatin1String("Qt4ProjectManager.Qt4RunConfiguration")
                || r->id() == QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration")) {
                deviceName = QLatin1String("Desktop");
                sshParameters.host = QLatin1String("127.0.0.1");
                sshParameters.port = 5656;
                sshParameters.userName.clear();
                sshParameters.password.clear();
                sshParameters.privateKeyFile.clear();
                testDeviceType = QLatin1String("Desktop");//Qt4Test::TestController::Desktop;
                return false;
            }
        }
    }
    deviceName = QLatin1String("Unknown");
    sshParameters.host = QLatin1String("0.0.0.0");
    sshParameters.port = 0;
    sshParameters.userName.clear();
    sshParameters.password.clear();
    sshParameters.privateKeyFile.clear();
    testDeviceType = QLatin1String("Desktop"); //Qt4Test::TestController::Desktop;

    return false;
}

void TestConfig::loadLine(QTextStream *s, const QString &id, QString &value)
{
    Q_UNUSED(id);

    QString tmp = s->readLine();
    int pos = tmp.indexOf(QLatin1Char('='));
    value = tmp.mid(pos + 1);
}

void TestConfig::loadLine(QTextStream *s, const QString &id, int &value)
{
    Q_UNUSED(id);

    QString tmp = s->readLine();
    int pos = tmp.indexOf(QLatin1Char('='));
    value = tmp.mid(pos + 1).toInt();
}

bool TestConfig::load(uint version, QTextStream *s)
{
    Q_UNUSED(version);

    if (s == 0)
        return false;

    QString tmp = s->readLine();
    if (tmp == QLatin1String("CONFIG-START")) {
        loadLine(s, QString(), m_configName);
        loadLine(s, QString(), m_runParams);
        loadLine(s, QString(), m_postprocessScript);
        loadLine(s, QString(), m_runScript);
        loadLine(s, QString(), m_lastTestcase);
        loadLine(s, QString(), m_lastTestfunc);
        QString tmp;
        loadLine(s, QString(), tmp);
        m_lastSelectedTests = tmp.split(QLatin1Char(','), QString::SkipEmptyParts);
        if (version > 1) {
            loadLine(s, QString(), m_uploadChange);
            loadLine(s, QString(), m_uploadBranch);
            loadLine(s, QString(), m_uploadPlatform);
            if (version < 8)
                tmp = s->readLine();
        }
        if (version > 2) {
            if (version < 8) tmp = s->readLine();
            tmp = s->readLine();
            m_autodetectPlatformConfiguration = (tmp == QLatin1String("AUTO-PLATFORM=1"));
            loadLine(s, QString(), m_uploadMethod);
        }
        if (version > 3) {
            loadLine(s, QString(), tmp);
            m_extraTests = tmp.split(QLatin1Char(':'), QString::SkipEmptyParts);
        }
        if (version > 4) {
            int tmp;
            loadLine(s, QString(), tmp);
            m_uploadMode = static_cast<UploadMode>(tmp);
        }
        if (version > 5 && version < 7) {
            QString dummy;
            loadLine(s, QString(), dummy);
            loadLine(s, QString(), dummy);
            loadLine(s, QString(), dummy);
        }
        if (version >= 9) {
            loadLine(s, QString(), m_qmakeMkspec);
            loadLine(s, QString(), m_uploadBranchSpecialization);
            loadLine(s, QString(), m_qmakeMkspecSpecialization);
        }
        //Add member here

        QString checkSum;
        loadLine(s, QString(), checkSum);
        return checkSum == QLatin1String("CONFIG-END");
    }
    return false;
}

void TestConfig::saveLine(QTextStream *s, const QString &id, const QString &value)
{
    *s << id << '=' << value << '\n';
}

bool TestConfig::save(QTextStream *s)
{
    if (s == 0)
        return false;

    QString checkSum = QLatin1String("CONFIG-START");
    *s << checkSum << QLatin1Char('\n');
    saveLine(s, QLatin1String("NAME"), m_configName);
    saveLine(s, QLatin1String("RUN_PARAMS"), m_runParams);
    saveLine(s, QLatin1String("POSTPROCESS_SCRIPT"), m_postprocessScript);
    saveLine(s, QLatin1String("RUN_SCRIPT"), m_runScript);
    saveLine(s, QLatin1String("LAST_TESTCASE"), m_lastTestcase);
    saveLine(s, QLatin1String("LAST_TESTFUNC"), m_lastTestfunc);
    saveLine(s, QLatin1String("LAST_SELECTED_TESTS"), m_lastSelectedTests.join(QString(QLatin1Char(','))));
    saveLine(s, QLatin1String("CHANGE"), m_uploadChange);
    saveLine(s, QLatin1String("BRANCH"), m_uploadBranch);
    saveLine(s, QLatin1String("PLATFORM"), m_uploadPlatform);
    saveLine(s, QLatin1String("AUTO-PLATFORM"), QString::fromLatin1("%1").arg(m_autodetectPlatformConfiguration));
    saveLine(s, QLatin1String("UPLOAD_METHOD"), m_uploadMethod);
    saveLine(s, QLatin1String("EXTRA_TESTS"), m_extraTests.join(QString(QLatin1Char(':'))));
    saveLine(s, QLatin1String("UPLOAD_MODE"), QString::number(static_cast<int>(m_uploadMode)));
    saveLine(s, QLatin1String("QMAKESPEC"), m_qmakeMkspec);
    saveLine(s, QLatin1String("BRANCH_SPECIALIZATION"), m_uploadBranchSpecialization);
    saveLine(s, QLatin1String("QMAKESPEC_SPECIALIZATION"), m_qmakeMkspecSpecialization);
    //Add member here

    checkSum = QLatin1String("CONFIG-END");
    *s << checkSum << '\n';
    return true;
}

void TestConfig::setCurrentTest(const QString &testcase, const QString &testfunc)
{
    m_lastTestcase = testcase;
    m_lastTestfunc = testfunc;
}

void TestConfig::setSelectedTests(const QStringList &list)
{
    m_lastSelectedTests = list;
}

QString TestConfig::currentTestCase()
{
    return m_lastTestcase;
}

QString TestConfig::currentTestFunc()
{
    return m_lastTestfunc;
}

QStringList TestConfig::selectedTests()
{
    return m_lastSelectedTests;
}

QString TestConfig::configName()
{
    return m_configName;
}

QString TestConfig::uploadChange()
{
    if (m_autodetectPlatformConfiguration && !m_changeDetected) {
        QProcess proc;
        proc.setEnvironment(*buildEnvironment());
        proc.setWorkingDirectory(srcPath());

        proc.start(QLatin1String("git"), QStringList() << QLatin1String("log") << QLatin1String("-1") << QLatin1String("--pretty=oneline"));
        bool ok = proc.waitForStarted();
        if (ok)
            ok = proc.waitForFinished();

        if (ok) {
            QString output = proc.readAllStandardOutput();
            if (!output.isEmpty()) {
                QStringList tmp = output.split(QLatin1Char(' '));
                if (tmp.count() >= 2) {
                    m_uploadChange = tmp[0];
                    m_changeDetected = true;
                }
            }
        }
    }
    return m_uploadChange;
}

void TestConfig::setUploadChange(const QString &newValue)
{
    m_uploadChange = newValue;
}

QString TestConfig::uploadBranch()
{
    if (m_autodetectPlatformConfiguration && !m_branchDetected) {
        QProcess proc;
        proc.setEnvironment(*buildEnvironment());
        proc.setWorkingDirectory(srcPath());

        proc.start(QLatin1String("git"), QStringList(QLatin1String("branch")));
        bool ok = proc.waitForStarted();
        if (ok)
            ok = proc.waitForFinished();

        if (ok) {
            QString output = proc.readAllStandardOutput();
            if (!output.isEmpty()) {
                QStringList tmp = output.split(QLatin1Char('\n'));
                foreach (const QString &line, tmp) {
                    if (line.startsWith(QLatin1Char('*'))) {
                        m_uploadBranch = line.mid(2);
                        m_branchDetected = true;
                    }
                }
            }
        }

        if (m_branchDetected){
            // prepend <Product>- to the branch name
            // this only supports remotes using git@... and git://
            proc.start(QLatin1String("git"), QStringList() << QLatin1String("remote") << QLatin1String("-v"));
            ok = proc.waitForStarted();
            if (ok)
                ok = proc.waitForFinished();

            if (ok) {
                QString output = proc.readAllStandardOutput();
                if (!output.isEmpty()) {
                    QRegExp gitRemoteRegEx(QLatin1String("(\\w+\\s+\\w+@[^:]+):(\\w+)/\\(.*)"));
                    QRegExp gitReadonlyRemoteRegEx(QLatin1String("(\\w+\\s+\\w+://.+)/(\\w+)/(.*)"));
                    QStringList tmp = output.split(QLatin1Char('\n'));
                    foreach (const QString &line, tmp) {
                        if (gitRemoteRegEx.exactMatch(line)) {
                            m_uploadBranch = gitRemoteRegEx.capturedTexts()[2]
                                + QLatin1Char('-') + m_uploadBranch;
                            break;
                        }
                        if (gitReadonlyRemoteRegEx.exactMatch(line)) {
                            m_uploadBranch = gitReadonlyRemoteRegEx.capturedTexts()[2]
                                + QLatin1Char('-') + m_uploadBranch;
                            break;
                        }
                    }
                }
            }
        }

    }
    return m_uploadBranch;
}

void TestConfig::setUploadBranch(const QString &newValue)
{
    m_uploadBranch = newValue;
}

QString TestConfig::uploadBranchSpecialization() const
{
    return m_uploadBranchSpecialization;
}

void TestConfig::setUploadBranchSpecialization(const QString &newValue)
{
    m_uploadBranchSpecialization = newValue;
}


QString TestConfig::uploadPlatform()
{
    if (m_autodetectPlatformConfiguration && !m_platformDetected) {
        m_uploadPlatform = QSystem::OSName();
        m_platformDetected = true;
    }
    return m_uploadPlatform;
}

void TestConfig::setUploadPlatform(const QString &newValue)
{
    m_uploadPlatform = newValue;
}

bool TestConfig::autoDetectPlatformConfiguration() const
{
    return m_autodetectPlatformConfiguration;
}

void TestConfig::setAutoDetectPlatformConfiguration(bool doAutoDetect)
{
    // only reset TestConfig values when auto detect is re-enabled
    if ((doAutoDetect != m_autodetectPlatformConfiguration) && doAutoDetect) {
        m_platformDetected = false;
        m_uploadPlatform.clear();
        m_changeDetected = false;
        m_uploadChange.clear();
        m_branchDetected = false;
        m_uploadBranch.clear();
        m_qmakeMkspec.clear();
    }
    m_autodetectPlatformConfiguration = doAutoDetect;
}

bool TestConfig::uploadUsingScp()
{
    return m_uploadMethod == QLatin1String("SCP");
}

bool TestConfig::uploadUsingEMail()
{
    return m_uploadMethod != QLatin1String("SCP");
}

void TestConfig::setUploadMethod(bool useScp)
{
    if (useScp)
        m_uploadMethod = QLatin1String("SCP");
    else
        m_uploadMethod = QLatin1String("EMAIL");
}

void TestConfig::deactivate()
{
    if (m_activeProject) {
        ProjectExplorer::BuildConfiguration *b =
            m_activeProject->activeTarget()->activeBuildConfiguration();
        disconnect(b, SIGNAL(buildDirectoryChanged()),
            this, SLOT(onBuildDirectoryChanged()));
        disconnect(m_activeProject, SIGNAL(environmentChanged()),
            this, SLOT(onEnvironmentChanged()));
    }
    m_activeProject = 0;
    m_currentLoadedSrcDir.clear();
    m_currentLoadedExtraDirs.clear();
    m_buildEnvironment.clear();
}

void TestConfig::activate(ProjectExplorer::Project *project)
{
    if (!project)
        return;
    m_activeProject = project;

    ProjectExplorer::BuildConfiguration *b = project->activeTarget()->activeBuildConfiguration();
    connect(b, SIGNAL(buildDirectoryChanged()), this, SLOT(onBuildDirectoryChanged()));
    connect(m_activeProject, SIGNAL(environmentChanged()), this, SLOT(onEnvironmentChanged()));
    connect(m_activeProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(onActiveTargetChanged()));
    onProjectSettingsChanged();
}

void TestConfig::onBuildDirectoryChanged()
{
    onProjectSettingsChanged();
}

void TestConfig::onEnvironmentChanged()
{
    m_buildEnvironment.clear();
}

void TestConfig::onActiveTargetChanged()
{
    m_buildEnvironment.clear();
}

void TestConfig::onProjectSettingsChanged()
{
    if (!m_activeProject)
        return;

    Core::ModeManager *mgr = Core::ModeManager::instance();
    if ((mgr && (mgr->currentMode()->id() != QLatin1String("Edit"))) || TestConfigurations::instance().updatesDelayed()) {
        // Try again later.
        emitConfigChanged();
        return;
    }

    TestCollection testCollection;
    bool rescanTests = (testCollection.count() == 0);
    if (m_currentLoadedSrcDir != srcPath() || rescanTests) {
        testCollection.removePath(m_currentLoadedSrcDir);
        testCollection.addPath(srcPath());
        m_currentLoadedSrcDir = srcPath();
    }

    if (m_currentLoadedExtraDirs != extraTests() || rescanTests) {
        foreach (const QString &dir, m_currentLoadedExtraDirs)
            testCollection.removePath(dir);
        foreach (const QString &dir, extraTests()) {
            if (!dir.isEmpty()) {
                testCollection.addExtraPath(dir);
            }
        }
        m_currentLoadedExtraDirs = extraTests();
    }

    onEnvironmentChanged();
}

QStringList *TestConfig::buildEnvironment()
{
    if (m_activeProject && m_activeProject->activeTarget()) {
        ProjectExplorer::BuildConfiguration *b =
            m_activeProject->activeTarget()->activeBuildConfiguration();
        if (b)
            m_buildEnvironment = b->environment().toStringList();
    }
    return &m_buildEnvironment;
}

QStringList *TestConfig::runEnvironment()
{
    if (m_activeProject && m_activeProject->activeTarget()) {
        ProjectExplorer::RunConfiguration *rc =
            m_activeProject->activeTarget()->activeRunConfiguration();
        if (rc) {
            if (rc->id() == QLatin1String("Qt4ProjectManager.Qt4RunConfiguration")) {
                m_runEnvironment =
                    static_cast<Qt4ProjectManager::Internal::Qt4RunConfiguration*>(rc)->environment().toStringList();
            } else if (rc->id() == QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration")) {
                m_runEnvironment =
                    static_cast<ProjectExplorer::CustomExecutableRunConfiguration*>(rc)->environment().toStringList();
            } else if (rc->id() == QLatin1String("RemoteLinuxRunConfiguration")) {
                m_runEnvironment =
                    static_cast<RemoteLinux::RemoteLinuxRunConfiguration *>(rc)->environment().toStringList();
            }
        }
    }
    return &m_runEnvironment;
}

QString TestConfig::buildPath()
{
    if (!m_activeProject)
        return QString();
    ProjectExplorer::BuildConfiguration *b =
        m_activeProject->activeTarget()->activeBuildConfiguration();
    if (!b)
        return QString();
    return QDir::convertSeparators(b->buildDirectory());
}

QString TestConfig::srcPath()
{
    if (!m_activeProject)
        return QString();
    return QDir::convertSeparators(m_activeProject->projectDirectory());
}

QStringList TestConfig::extraTests() const
{
    return m_extraTests;
}

void TestConfig::setExtraTests(const QStringList &list)
{
    m_extraTests = list;
    emitConfigChanged();
}

void TestConfig::emitConfigChanged()
{
    // Wait a little with emitting 'configChanged' until we have stopped typing
    m_configChangedTimer.stop();
    m_configChangedTimer.setSingleShot(true);
    m_configChangedTimer.start(500);
}

// --------------------------------------------------------------------

TestConfigurations::TestConfigurations()
{
    Q_ASSERT(!m_instance);

    m_instance = this;

    d = new TestConfigurationsPrivate;
    connect(d, SIGNAL(activeConfigurationChanged()), this, SIGNAL(activeConfigurationChanged()));
    connect(d, SIGNAL(testSelectionChanged(QStringList, QObject*)),
        this, SIGNAL(testSelectionChanged(QStringList, QObject*)));
}

TestConfigurations::~TestConfigurations()
{
    m_instance = 0;
    delete d;
}

TestConfigurations &TestConfigurations::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

TestConfig *TestConfigurations::activeConfiguration()
{
    Q_ASSERT(d);
    return d->activeConfiguration();
}

void TestConfigurations::setActiveConfiguration(ProjectExplorer::Project *project)
{
    Q_ASSERT(d);
    return d->setActiveConfiguration(project);
}

TestConfig *TestConfigurations::config(const QString &cfgName)
{
    Q_ASSERT(d);
    return d->config(cfgName);
}

TestConfig *TestConfigurations::findConfig(const QString &srcPath)
{
    Q_ASSERT(d);
    return d->findConfig(srcPath);
}

QStringList TestConfigurations::selectedTests()
{
    Q_ASSERT(d);
    return d->selectedTests();
}

void TestConfigurations::setSelectedTests(const QStringList &list)
{
    Q_ASSERT(d);
    d->setSelectedTests(list);
}

void TestConfigurations::setCurrentTest(const QString &testCase, const QString &testFunction)
{
    Q_ASSERT(d);
    d->setCurrentTest(testCase,testFunction);
}

QString TestConfigurations::currentTestCase()
{
    Q_ASSERT(d);
    return d->currentTestCase();
}

QString TestConfigurations::currentTestFunction()
{
    Q_ASSERT(d);
    return d->currentTestFunction();
}

void TestConfigurations::rescan()
{
    Q_ASSERT(d);
    d->rescan();
}

void TestConfigurations::delayConfigUpdates(bool delay)
{
    Q_ASSERT(d);
    d->delayConfigUpdates(delay);
}

bool TestConfigurations::updatesDelayed()
{
    Q_ASSERT(d);
    return d->updatesDelayed();
}

void TestConfigurations::onActiveConfigurationChanged()
{
    Q_ASSERT(d);
    d->onActiveConfigurationChanged();
}

void TestConfigurations::onTestSelectionChanged(const QStringList &selection)
{
    Q_ASSERT(d);
    d->onTestSelectionChanged(selection, sender());
}
