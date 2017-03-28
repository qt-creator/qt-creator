/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "testwizard.h"
#include "testwizarddialog.h"

#include <cpptools/abstracteditorsupport.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QTextStream>
#include <QFileInfo>

namespace QmakeProjectManager {
namespace Internal {

TestWizard::TestWizard()
{
    setId("L.Qt4Test");
    setCategory(QLatin1String(ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
             ProjectExplorer::Constants::QT_PROJECT_WIZARD_CATEGORY_DISPLAY));
    setDisplayName(tr("Qt Unit Test"));
    setDescription(tr("Creates a QTestLib-based unit test for a feature or a class. "
                "Unit tests allow you to verify that the code is fit for use "
                "and that there are no regressions."));
    setIcon(QIcon(QLatin1String(":/wizards/images/console.png")));
    setRequiredFeatures({QtSupport::Constants::FEATURE_QT_CONSOLE, QtSupport::Constants::FEATURE_QT_PREFIX});
}

Core::BaseFileWizard *TestWizard::create(QWidget *parent, const Core::WizardDialogParameters &parameters) const
{
    TestWizardDialog *dialog = new TestWizardDialog(this, displayName(), icon(), parent, parameters);
    dialog->setProjectName(TestWizardDialog::uniqueProjectName(parameters.defaultPath()));
    return dialog;
}

// ---------------- code generation helpers
static const char initTestCaseC[] = "initTestCase";
static const char cleanupTestCaseC[] = "cleanupTestCase";
static const char closeFunctionC[] = "}\n\n";
static const char testDataTypeC[] = "QString";

static inline void writeVoidMemberDeclaration(QTextStream &str,
                                              const QString &indent,
                                              const QString &methodName)
{
    str << indent << "void " << methodName << "();\n";
}

static inline void writeVoidMemberBody(QTextStream &str,
                                       const QString &className,
                                       const QString &methodName,
                                       bool close = true)
{
    str << "void " << className << "::" << methodName << "()\n{\n";
    if (close)
        str << closeFunctionC;
}

static QString generateTestCode(const TestWizardParameters &testParams,
                                const QString &sourceBaseName)
{
    QString rc;
    const QString indent = QString(4, QLatin1Char(' '));
    QTextStream str(&rc);
    // Includes
    str << CppTools::AbstractEditorSupport::licenseTemplate(testParams.fileName, testParams.className)
        << "#include <QString>\n#include <QtTest>\n";
    if (testParams.requiresQApplication)
        str << "#include <QCoreApplication>\n";
    // Class declaration
    str  << "\nclass " << testParams.className << " : public QObject\n"
        "{\n" << indent << "Q_OBJECT\n\npublic:\n"
        << indent << testParams.className << "();\n\nprivate Q_SLOTS:\n";
    if (testParams.initializationCode) {
        writeVoidMemberDeclaration(str, indent, QLatin1String(initTestCaseC));
        writeVoidMemberDeclaration(str, indent, QLatin1String(cleanupTestCaseC));
    }
    const QString dataSlot = testParams.testSlot + QLatin1String("_data");
    if (testParams.useDataSet)
        writeVoidMemberDeclaration(str, indent, dataSlot);
    writeVoidMemberDeclaration(str, indent, testParams.testSlot);
    str << "};\n\n";
    // Code: Constructor
    str << testParams.className << "::" << testParams.className << "()\n{\n}\n\n";
    // Code: Initialization slots
    if (testParams.initializationCode) {
        writeVoidMemberBody(str, testParams.className, QLatin1String(initTestCaseC));
        writeVoidMemberBody(str, testParams.className, QLatin1String(cleanupTestCaseC));
    }
    // test data generation slot
    if (testParams.useDataSet) {
        writeVoidMemberBody(str, testParams.className, dataSlot, false);
        str << indent << "QTest::addColumn<" << testDataTypeC << ">(\"data\");\n"
            << indent << "QTest::newRow(\"0\") << " << testDataTypeC << "();\n"
            << closeFunctionC;
    }
    // Test slot with data or dummy
    writeVoidMemberBody(str, testParams.className, testParams.testSlot, false);
    if (testParams.useDataSet)
        str << indent << "QFETCH(" << testDataTypeC << ", data);\n";
    switch (testParams.type) {
    case TestWizardParameters::Test:
        str << indent << "QVERIFY2(true, \"Failure\");\n";
        break;
    case TestWizardParameters::Benchmark:
        str << indent << "QBENCHMARK {\n" << indent << "}\n";
        break;
    }
    str << closeFunctionC;
    // Main & moc include
    str << (testParams.requiresQApplication ? "QTEST_MAIN" : "QTEST_APPLESS_MAIN")
        << '(' << testParams.className << ")\n\n"
        << "#include \"" << sourceBaseName << ".moc\"\n";
    return rc;
}

Core::GeneratedFiles TestWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const TestWizardDialog *wizardDialog = qobject_cast<const TestWizardDialog *>(w);
    QTC_ASSERT(wizardDialog, return Core::GeneratedFiles());

    const QtProjectParameters projectParams = wizardDialog->projectParameters();
    const TestWizardParameters testParams = wizardDialog->testParameters();
    const QString projectPath = projectParams.projectPath();

    // Create files: Source
    const QString sourceFilePath = Core::BaseFileWizardFactory::buildFileName(projectPath, testParams.fileName, sourceSuffix());
    const QFileInfo sourceFileInfo(sourceFilePath);

    Core::GeneratedFile source(sourceFilePath);
    source.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    source.setContents(generateTestCode(testParams, sourceFileInfo.baseName()));

    // Create profile with define for base dir to find test data
    const QString profileName = Core::BaseFileWizardFactory::buildFileName(projectPath, projectParams.fileName, profileSuffix());
    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    QString contents;
    {
        QTextStream proStr(&contents);
        QtProjectParameters::writeProFileHeader(proStr);
        projectParams.writeProFile(proStr);
        proStr << "\n\nSOURCES +="
               << " \\\n        " << Utils::FileName::fromString(sourceFilePath).fileName()
               << " \n\nDEFINES += SRCDIR=\\\\\\\"$$PWD/\\\\\\\"\n";
    }
    profile.setContents(contents);

    return Core::GeneratedFiles() <<  source << profile;
}

} // namespace Internal
} // namespace QmakeProjectManager
