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

#ifndef TESTSETTINGS_H
#define TESTSETTINGS_H

#include <QObject>

// class TestSettingsPrivate;
class TestSettingsPrivate : public QObject
{
    Q_OBJECT

public:
    TestSettingsPrivate();
    ~TestSettingsPrivate();

    void load();
    void save();
    void emitChanged();

    bool m_showPassedResults;
    bool m_showDebugResults;
    bool m_showSkippedResults;
    int m_showVerbose;
    QString m_uploadServer;
    QString m_systemTestRunner;
    int m_learnMode;
    int m_autosaveOn;
    int m_componentViewMode;
    int m_hiddenTestTypes;

signals:
    void changed();
};

class TestSettings : public QObject
{
    Q_OBJECT

public:
    TestSettings();
    virtual ~TestSettings();

    void save();

    bool showPassedResults();
    bool showDebugResults();
    bool showSkippedResults();
    void setShowPassedResults(bool doShow);
    void setShowDebugResults(bool doShow);
    void setShowSkippedResults(bool doShow);

    bool showVerbose();
    void setShowVerbose(bool doVerbose);

    bool autosaveOn();
    void setAutosaveOn(bool on);

    QString systemTestRunner() const;
    void setSystemTestRunner(const QString &newValue);

    QString uploadServer() const;
    void setUploadServer(const QString &newValue);

    bool componentViewMode();
    void setComponentViewMode(bool on);
    int hiddenTestTypes();
    void setHiddenTestTypes(int);

    int learnMode(); // 0 = none, 1 = new, 2 = all;

    void setLearnMode(int newMode);

signals:
    void changed();

private:
    static TestSettingsPrivate *d;
    static int m_refCount;
};

#endif
