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

#include "testsettings.h"

#include <QDir>
#include <QString>
#include <QStringList>

#include <QFile>
#include <QTextStream>
#include <QDateTime>

class TestIniFile
{
public:
    TestIniFile(const QString &fileName);
    virtual ~TestIniFile();

    bool read(const QString &key, QString &ret);
    bool read(const QString &key, int &ret);
    bool read(const QString &key, uint &ret);
    bool read(const QString &key, QDateTime &ret);

    bool write(const QString &comment, const QString &key, const QString &value);
    bool write(const QString &comment, const QString &key, int value);
    bool write(const QString &comment, const QString &key, uint value);
    bool write(const QString &comment, const QString &key, QDateTime value);

private:
    bool initSaving(const QString &comment);
    bool find(const QString &key, QString &line);

    QStringList m_settings;
    QString m_fileName;

    QFile *m_outFile;
    QTextStream *m_outStream;
};

#include <iostream>
using namespace std;

TestIniFile::TestIniFile(const QString &fileName)
{
    m_outFile = 0;
    m_outStream = 0;

    m_fileName = fileName;

    QFile F(m_fileName);
    if (F.open(QIODevice::ReadOnly)) {
        QTextStream ds(&F);
        QString S;
        while (!ds.atEnd()) {
            S = ds.readLine().trimmed();
            if (!S.isEmpty() && !S.startsWith(QLatin1Char('#')))
                m_settings.append(S);
        }
    }
}

TestIniFile::~TestIniFile()
{
    if (m_outFile != 0) {
        delete m_outStream;
        delete m_outFile;
    }
}

bool TestIniFile::find(const QString &key, QString &line)
{
    foreach (const QString &S, m_settings) {
        if (S.startsWith(key)) {
            int pos = S.indexOf(QLatin1Char('='));
            if (pos > 0) {
                line = S.mid(pos+1).trimmed();
                return true;
            }
        }
    }
    return false;
}

bool TestIniFile::read(const QString &key, QString &ret)
{
    QString line;
    if (find(key, line)) {
        ret = line;
        return true;
    }

    return false;
}

bool TestIniFile::read(const QString &key, int &ret)
{
    QString line;
    if (find(key, line)) {
        bool ok;
        ret = line.toInt(&ok);
        return ok;
    }

    return false;
}

bool TestIniFile::read(const QString &key, uint &ret)
{
    QString line;
    if (find(key, line)) {
        bool ok;
        ret = line.toUInt(&ok);
        return ok;
    }

    return false;
}

bool TestIniFile::read(const QString &key, QDateTime &ret)
{
    QString line;
    if (find(key, line)) {
        if (line.isEmpty())
            ret = QDateTime();
        else
            ret = QDateTime::fromString(line, Qt::ISODate);
        return ret.isValid();
    }

    return false;
}

bool TestIniFile::initSaving(const QString &comment)
{
    if (m_outFile == 0) {
        m_outFile = new QFile(m_fileName);
        if (m_outFile->open(QIODevice::WriteOnly)) {
            m_outStream = new QTextStream(m_outFile);
            *m_outStream << "#Lines with a # on the first position are remarks.\n";
            *m_outStream << "#data on all other lines must be entered according to the hints.\n";
            *m_outStream << "\n";
        } else {
            delete m_outFile;
            m_outFile = 0;
            return false;
        }
    }

    foreach (const QString &S, comment.split(QLatin1Char('\n'))) {
        if (!S.startsWith(QLatin1Char('#')))
            *m_outStream << "# ";
        *m_outStream << S << "\n";
    }

    return true;
}

bool TestIniFile::write(const QString &comment, const QString &key, const QString &value)
{
    if (!initSaving(comment))
        return false;
    *m_outStream << key << '=';
    if (!value.isEmpty())
        *m_outStream << value;
    *m_outStream << "\n\n";
    return true;
}

bool TestIniFile::write(const QString &comment, const QString &key, int value)
{
    if (!initSaving(comment))
        return false;
    *m_outStream << key << '=' << QString::number(value) << "\n\n";
    return true;
}

bool TestIniFile::write(const QString &comment, const QString &key, uint value)
{
    if (!initSaving(comment))
        return false;
    *m_outStream << key << '=' << QString::number(value) << "\n\n";
    return true;
}

bool TestIniFile::write(const QString &comment, const QString &key, QDateTime value)
{
    if (!initSaving(comment))
        return false;
    *m_outStream << key << '=';
    if (value.isValid())
        *m_outStream << value.toString(Qt::ISODate);
    *m_outStream << "\n\n";
    return true;
}

int TestSettings::m_refCount = 0;
TestSettingsPrivate *TestSettings::d = 0;

TestSettingsPrivate::TestSettingsPrivate()
{
    // Clear out the settings first incase there is no ini file or it's missing values
    m_showPassedResults = true;
    m_showDebugResults = true;
    m_showSkippedResults = true;
    m_showVerbose = 0;
    m_hiddenTestTypes = 0;
    load();
}

TestSettingsPrivate::~TestSettingsPrivate()
{
    save();
}

void TestSettingsPrivate::emitChanged()
{
    emit changed();
}

void TestSettingsPrivate::load()
{
    TestIniFile ini(QDir ::homePath() + QDir::separator() + ".qttest"
        + QDir::separator() + "saved_settings");

    int showPassedResults;
    int showDebugResults;
    int showSkippedResults;
    ini.read("showPassedResults", showPassedResults);
    ini.read("showDebugResults", showDebugResults);
    ini.read("showSkippedResults", showSkippedResults);
    ini.read("showVerbose", m_showVerbose);
    ini.read("autosaveOn", m_autosaveOn);
    ini.read("componentViewMode", m_componentViewMode);
    ini.read("uploadServer", m_uploadServer);
    ini.read("systemTestRunner", m_systemTestRunner);
    ini.read("learnMode", m_learnMode);
    ini.read("hiddenTestTypes", m_hiddenTestTypes);

    m_showPassedResults = showPassedResults;
    m_showDebugResults = showDebugResults;
    m_showSkippedResults = showSkippedResults;
}

void TestSettingsPrivate::save()
{
    QDir().mkpath(QDir::homePath() + QDir::separator() + ".qttest");
    TestIniFile ini(QDir::homePath() + QDir::separator() + ".qttest" + QDir::separator() + "saved_settings");

    ini.write("Set to 1 if passed must be shown", "showPassedResults", m_showPassedResults);
    ini.write("Set to 1 if debug must be shown", "showDebugResults", m_showDebugResults);
    ini.write("Set to 1 if skipped must be shown", "showSkippedResults", m_showSkippedResults);
    ini.write("Set to 1 for verbose logging", "showVerbose", m_showVerbose);
    ini.write("Autosaving enabled/disabled", "autosaveOn", m_autosaveOn);
    ini.write("Show tests in selection tree sorted by component", "componentViewMode", m_componentViewMode);
    ini.write("The server to which test results will be uploaded", "uploadServer", m_uploadServer);
    ini.write("The fully qualified path to the system test executer", "systemTestRunner", m_systemTestRunner);
    ini.write("The current learn mode", "learnMode", m_learnMode);
    ini.write("Test types hidden in selection tree", "hiddenTestTypes", m_hiddenTestTypes);
}

TestSettings::TestSettings()
{
    if (m_refCount++ == 0) {
        d = new TestSettingsPrivate();
        Q_ASSERT(d);
        d->load();
    }
    connect(d, SIGNAL(changed()), this, SIGNAL(changed()));
}

TestSettings::~TestSettings()
{
    if (--m_refCount == 0) {
        delete d;
        d = 0;
    }
}

void TestSettings::save()
{
    Q_ASSERT(d);
    d->save();
}

bool TestSettings::showPassedResults()
{
    Q_ASSERT(d);
    return d->m_showPassedResults;
}

bool TestSettings::showDebugResults()
{
    Q_ASSERT(d);
    return d->m_showDebugResults;
}

bool TestSettings::showSkippedResults()
{
    Q_ASSERT(d);
    return d->m_showSkippedResults;
}

void TestSettings::setShowPassedResults(bool doShow)
{
    Q_ASSERT(d);
    if (d->m_showPassedResults != doShow) {
        d->m_showPassedResults = doShow;
        d->emitChanged();
    }
}

void TestSettings::setShowDebugResults(bool doShow)
{
    Q_ASSERT(d);
    if (d->m_showDebugResults != doShow) {
        d->m_showDebugResults = doShow;
        d->emitChanged();
    }
}

void TestSettings::setShowSkippedResults(bool doShow)
{
    Q_ASSERT(d);
    if (d->m_showSkippedResults != doShow) {
        d->m_showSkippedResults = doShow;
        d->emitChanged();
    }
}

bool TestSettings::showVerbose()
{
    Q_ASSERT(d);
    return (d->m_showVerbose != 0);
}

void TestSettings::setShowVerbose(bool doVerbose)
{
    Q_ASSERT(d);
    d->m_showVerbose = 0;
    if (doVerbose)
        d->m_showVerbose = 1;
}

bool TestSettings::autosaveOn()
{
    Q_ASSERT(d);
    return (d->m_autosaveOn != 0);
}

void TestSettings::setAutosaveOn(bool on)
{
    Q_ASSERT(d);
    if (on)
        d->m_autosaveOn = 1;
    else
        d->m_autosaveOn = 0;
}

bool TestSettings::componentViewMode()
{
    Q_ASSERT(d);
    return (d->m_componentViewMode != 0);
}

void TestSettings::setComponentViewMode(bool on)
{
    Q_ASSERT(d);
    if (on)
        d->m_componentViewMode = 1;
    else
        d->m_componentViewMode = 0;
}

int TestSettings::hiddenTestTypes()
{
    Q_ASSERT(d);
    return d->m_hiddenTestTypes;
}

void TestSettings::setHiddenTestTypes(int types)
{
    Q_ASSERT(d);
    d->m_hiddenTestTypes = types;
}

QString TestSettings::uploadServer() const
{
    Q_ASSERT(d);
    return d->m_uploadServer;
}

void TestSettings::setUploadServer(const QString &newValue)
{
    Q_ASSERT(d);
    d->m_uploadServer = newValue;
}

QString TestSettings::systemTestRunner() const
{
    Q_ASSERT(d);
    return d->m_systemTestRunner;
}

void TestSettings::setSystemTestRunner(const QString &newValue)
{
    Q_ASSERT(d);
    d->m_systemTestRunner = newValue;
}

int TestSettings::learnMode()
{
    Q_ASSERT(d);
    return d->m_learnMode;
}

void TestSettings::setLearnMode(int newMode)
{
    Q_ASSERT(d);
    d->m_learnMode = newMode;
}
