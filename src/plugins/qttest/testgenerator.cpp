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

#include "testgenerator.h"
#include "testconfigurations.h"
#include "qsystem.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QDebug>

#include <stdlib.h>

TestGenerator::TestGenerator()
{
    m_initialized = false;
    m_enableComponentNamedTest = true;
}

/*!
    Generates a .pro file for the testCase specified in the constructor and saves
    it to the path specified in the constructor.
*/
bool TestGenerator::generateProFile()
{
    if (!m_initialized)
        return false;

    QFile F(m_generatedProname);
    if (F.open(QIODevice::WriteOnly)) {
        QTextStream src(&F);

        if (m_genMode != SystemTest) {
            src << "load(testcase)\n";
            src << "TARGET = " << m_testCase.toLower() << '\n';
            src << "QT += testlib\n";
            src << "SOURCES = " << m_testCase.toLower() << ".cpp\n";
        } else if (m_genMode == SystemTest) {
            src << "TEMPLATE = subdirs\n";
            src << "CONFIG += systemtest\n";
            src << "SOURCES = " << m_testCase.toLower() << ".qtt\n";
        }
        F.close();
        return true;
    }

    return false;
}

/*!
    Generates .pro files for the parent directories of the testCase.
*/
bool TestGenerator::generateParentDirectoryProFiles()
{
    if (!m_initialized)
        return false;

    QDir::cleanPath(m_rootDir);

    return true;
}

bool TestGenerator::generateTestCode()
{
    if (!m_initialized)
        return false;

    if (QDir().exists(m_generatedFilename)) {
        int ret = QMessageBox::warning(0, "Error", "File already exists.\nDo you wish to overwrite it?",
            QMessageBox::Yes|QMessageBox::No);
        if (ret == QMessageBox::No) {
            return false;
        } else {
            if (!QFile().remove(m_generatedFilename)) {
                QMessageBox::warning(0, "Error", "Could not remove file", QMessageBox::Ok);
                return false;
            }
        }
    }

    // create a source file that contains class definition, main implementation and class implementation
    QFile sourceFile(m_generatedFilename);
    if (!sourceFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(0, "Error", "Error creating source file", QMessageBox::Ok);
        return false;
    }
    QTextStream sourceFileStream(&sourceFile);
    if (m_genMode == UnitTest || m_genMode == PerformanceTest || m_genMode == IntegrationTest)
        initSrc(&sourceFileStream);
    else
        initScript(&sourceFileStream);
    sourceFile.close();

    return true;
}

void TestGenerator::initSrc(QTextStream *s)
{
    if (!m_initialized)
        return;

    TestConfig *cfg = TestConfigurations::instance().findConfig(m_generatedFilename);
    if (!cfg)
        return;

    // First we write the license stuff to the header file
    QFile crfile(cfg->copyrightHeader());
    if (crfile.open(QIODevice::ReadOnly)) {
        QTextStream crfileStream(&crfile);
        *s << crfileStream.readAll();
    }

    *s << '\n';
    *s << "//TESTED_COMPONENT=" << m_testedComponent << '\n';
    *s << "//TESTED_CLASS=" << m_testedClass << '\n';
    *s << "//TESTED_FILE=" << m_includeFile << '\n';
    *s << '\n';
    *s << "#include <QtTest/QtTest>\n";

    if (!m_includeFile.isEmpty())
        *s << "#include \"" << m_includeFile << "\"\n";

    *s << '\n';
    *s << "/*!\n";
    *s << "    \\internal\n";
    *s << '\n';
    *s << "    \\class " << m_testCase << '\n';
    *s << "    \\brief <put brief description here>\n";
    *s << '\n';
    *s << "    <put extended description here>\n";
    *s << '\n';

    if (m_testedClass.isEmpty())
        *s << "    \\sa " << "<tested class name>\n";
    else
        *s << "    \\sa " << m_testedClass << '\n';

    *s << "*/\n";
    *s << "class " << m_testCase << " : public QObject\n";
    *s << "{\n";
    *s << "    Q_OBJECT\n";
    *s << '\n';
    *s << "public:\n";
    *s << "    " << m_testCase << "();\n";
    *s << "    virtual ~" << m_testCase << "();\n";
    *s << '\n';
    *s << "private slots:\n";
    *s << "    virtual void initTestCase();\n";
    *s << "    virtual void init();\n";
    *s << "    virtual void cleanup();\n";
    *s << "    virtual void cleanupTestCase();\n";
    *s << "};\n";
    *s << '\n';

    *s << "QTEST_MAIN(" << m_testCase << ")\n";
    *s << "#include " << '"' << m_testCase.toLower() << ".moc" << '"' << '\n';
    *s << '\n';
    *s << '\n';

    *s << m_testCase << "::" << m_testCase << "()\n";
    *s << "{\n";
    *s << "}\n";
    *s << '\n';

    *s << m_testCase << "::~" << m_testCase << "()\n";
    *s << "{\n";
    *s << "}\n";
    *s << '\n';

    *s << "void " << m_testCase << "::initTestCase()\n";
    *s << "{\n";
    *s << "}\n";
    *s << '\n';
    *s << "void " << m_testCase << "::init()\n";
    *s << "{\n";
    *s << "}\n";
    *s << '\n';
    *s << "void " << m_testCase << "::cleanup()\n";
    *s << "{\n";
    *s << "}\n";
    *s << '\n';
    *s << "void " << m_testCase << "::cleanupTestCase()\n";
    *s << "{\n";
    *s << "}\n";
    *s << '\n';
    *s << '\n';
}

void TestGenerator::initScript(QTextStream *s)
{
    if (!m_initialized)
        return;

    TestConfig *cfg = TestConfigurations::instance().findConfig(m_generatedFilename);
    if (!cfg)
        return;

    QFile crfile(cfg->copyrightHeader());
    if (crfile.open(QIODevice::ReadOnly)) {
        QTextStream crfileStream(&crfile);
        *s << crfileStream.readAll();
    }

    *s << '\n';
    *s << "//TESTED_COMPONENT=" << m_testedComponent << '\n';
    *s << '\n';
    *s << "testcase = {\n";
    *s << "    initTestCase: function()\n";
    *s << "    {\n";
    *s << "    },\n";
    *s << '\n';
    *s << "    init: function()\n";
    *s << "    {\n";
    *s << "    },\n";
    *s << '\n';
    *s << "    cleanup: function()\n";
    *s << "    {\n";
    *s << "    },\n";
    *s << '\n';
    *s << "    cleanupTestCase: function()\n";
    *s << "    {\n";
    *s << "    },\n";
    *s << '\n';
    *s << '\n';
    *s << "}  // end of testcase\n";
    *s << '\n';
}

void TestGenerator::setTestCase(GenMode mode, const QString &rootDir, const QString &testDir,
    const QString &testCase, const QString &testedComponent, const QString &testedClassName,
    const QString &testedClassFile)
{
    Q_UNUSED(testedClassFile);
    QString testCaseNameSuffix;
    m_generatedFilename.clear();

    m_genMode = mode;
    m_testedClass = testedClassName;
    m_testedComponent = testedComponent;
    m_includeFile.clear();

    m_testCase = testCase;
    m_rootDir = rootDir;
    if (testDir.isEmpty()) {
        // common case , auto detect test dir value
        m_testDir = "auto";
        if (mode == TestGenerator::SystemTest)
            m_testDir = "systemtests";
        else if (mode == TestGenerator::PerformanceTest)
            m_testDir = "benchmarks";
    } else {
        m_testDir = testDir;
    }

    // use component name in test case directory path?
    if (m_enableComponentNamedTest)
        testCaseNameSuffix = testedComponent + QDir::separator();

    m_tcDir = m_rootDir + QDir::separator() + m_testDir + QDir::separator()
        + testCaseNameSuffix + m_testCase.toLower();

    QString fname(m_tcDir + QDir::separator() + m_testCase.toLower());
    if (m_genMode == UnitTest || m_genMode == IntegrationTest || m_genMode == PerformanceTest)
        fname += ".cpp";
    else
        fname += ".qtt";
    m_generatedFilename = fname;

    m_generatedProname = m_tcDir + QDir::separator() + m_testCase.toLower() + ".pro";

    m_initialized = true;
}

bool TestGenerator::generateTest()
{
    if (!m_initialized)
        return false;

    QDir().mkpath(m_tcDir);

    if (!QFile::exists(m_tcDir)){
       QMessageBox::warning(0, "Unable to create test directory",
            QString("Unable to create test directory %1").arg(m_tcDir));
       return false;
    }

    return (generateParentDirectoryProFiles() && generateProFile() && generateTestCode());
}

QString TestGenerator::generatedFileName()
{
    return m_generatedFilename;
}

QString TestGenerator::generatedProName()
{
    return m_generatedProname;
}

void TestGenerator::enableComponentInTestName(bool value)
{
    m_enableComponentNamedTest = value;
}

QString TestGenerator::testCaseDirectory()
{
    return m_tcDir;
}
