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

#ifndef TESTGENERATOR_H
#define TESTGENERATOR_H

#include <QString>

QT_FORWARD_DECLARE_CLASS(QTextStream)

class TestGenerator
{
public:
    TestGenerator();

    enum GenMode { UnitTest, SystemTest, IntegrationTest, PerformanceTest };
    void setTestCase(GenMode mode, const QString &rootDir, const QString &testDir, const QString &testCase,
        const QString &testedComponent, const QString &testedClassName, const QString &testedClassFile);
    bool generateTest();
    bool generateParentDirectoryProFiles();
    void enableComponentInTestName(bool value);

    QString generatedFileName();
    QString generatedProName();
    QString testCaseDirectory();

protected:
    void initSrc(QTextStream *s);
    void initScript(QTextStream *s);
    bool generateProFile();
    bool generateTestCode();

private:
    QString m_generatedFilename;
    QString m_generatedProname;
    QString m_testDir;
    QString m_rootDir;
    QString m_tcDir;
    QString m_testedClass;
    QString m_testedComponent;
    QString m_includeFile;
    QString m_testCase;
    GenMode m_genMode;
    bool m_initialized;
    bool m_enableComponentNamedTest;
};

#endif
