/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppclasswizard.h"
#include "cppeditorconstants.h"

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/abstracteditorsupport.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <utils/codegeneration.h>
#include <utils/newclasswidget.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QTextStream>
#include <QSettings>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QSpacerItem>
#include <QWizard>

using namespace CppEditor;
using namespace CppEditor::Internal;

// ========= ClassNamePage =========

ClassNamePage::ClassNamePage(QWidget *parent) :
    QWizardPage(parent),
    m_isValid(false)
{
    setTitle(tr("Enter Class Name"));
    setSubTitle(tr("The header and source file names will be derived from the class name"));

    m_newClassWidget = new Utils::NewClassWidget;
    // Order, set extensions first before suggested name is derived
    m_newClassWidget->setBaseClassInputVisible(true);
    m_newClassWidget->setBaseClassChoices(QStringList() << QString()
            << QLatin1String("QObject")
            << QLatin1String("QWidget")
            << QLatin1String("QMainWindow")
            << QLatin1String("QDeclarativeItem")
            << QLatin1String("QQuickItem"));
    m_newClassWidget->setBaseClassEditable(true);
    m_newClassWidget->setFormInputVisible(false);
    m_newClassWidget->setNamespacesEnabled(true);
    m_newClassWidget->setAllowDirectories(true);
    m_newClassWidget->setBaseClassInputVisible(true);

    connect(m_newClassWidget, SIGNAL(validChanged()), this, SLOT(slotValidChanged()));

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->addWidget(m_newClassWidget);
    QSpacerItem *vSpacer = new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding);
    pageLayout->addItem(vSpacer);

    initParameters();
}

// Retrieve settings of CppTools plugin.
static bool lowerCaseFiles()
{
    QString lowerCaseSettingsKey = QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP);
    lowerCaseSettingsKey += QLatin1Char('/');
    lowerCaseSettingsKey += QLatin1String(CppTools::Constants::LOWERCASE_CPPFILES_KEY);
    const bool lowerCaseDefault = CppTools::Constants::lowerCaseFilesDefault;
    return Core::ICore::settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

// Set up new class widget from settings
void ClassNamePage::initParameters()
{
    const Core::MimeDatabase *mdb = Core::ICore::mimeDatabase();
    m_newClassWidget->setHeaderExtension(mdb->preferredSuffixByType(QLatin1String(Constants::CPP_HEADER_MIMETYPE)));
    m_newClassWidget->setSourceExtension(mdb->preferredSuffixByType(QLatin1String(Constants::CPP_SOURCE_MIMETYPE)));
    m_newClassWidget->setLowerCaseFiles(lowerCaseFiles());
}

void ClassNamePage::slotValidChanged()
{
    const bool validNow = m_newClassWidget->isValid();
    if (m_isValid != validNow) {
        m_isValid = validNow;
        emit completeChanged();
    }
}

CppClassWizardDialog::CppClassWizardDialog(QWidget *parent) :
    Utils::Wizard(parent),
    m_classNamePage(new ClassNamePage(this))
{
    Core::BaseFileWizard::setupWizard(this);
    setWindowTitle(tr("C++ Class Wizard"));
    const int classNameId = addPage(m_classNamePage);
    wizardProgress()->item(classNameId)->setTitle(tr("Details"));
}

void CppClassWizardDialog::setPath(const QString &path)
{
    m_classNamePage->newClassWidget()->setPath(path);
}

CppClassWizardParameters  CppClassWizardDialog::parameters() const
{
    CppClassWizardParameters rc;
    const Utils::NewClassWidget *ncw = m_classNamePage->newClassWidget();
    rc.className = ncw->className();
    rc.headerFile = ncw->headerFileName();
    rc.sourceFile = ncw->sourceFileName();
    rc.baseClass = ncw->baseClassName();
    rc.path = ncw->path();
    rc.classType = ncw->classType();
    return rc;
}

// ========= CppClassWizard =========

CppClassWizard::CppClassWizard(const Core::BaseFileWizardParameters &parameters,
                               QObject *parent)
  : Core::BaseFileWizard(parameters, parent)
{
}

Core::FeatureSet CppClassWizard::requiredFeatures() const
{
    return Core::FeatureSet();
}

QString CppClassWizard::sourceSuffix() const
{
    return preferredSuffix(QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
}

QString CppClassWizard::headerSuffix() const
{
    return preferredSuffix(QLatin1String(Constants::CPP_HEADER_MIMETYPE));
}

QWizard *CppClassWizard::createWizardDialog(QWidget *parent,
                                            const Core::WizardDialogParameters &wizardDialogParameters) const
{
    CppClassWizardDialog *wizard = new CppClassWizardDialog(parent);
    foreach (QWizardPage *p, wizardDialogParameters.extensionPages())
        BaseFileWizard::applyExtensionPageShortTitle(wizard, wizard->addPage(p));
    wizard->setPath(wizardDialogParameters.defaultPath());
    return wizard;
}

Core::GeneratedFiles CppClassWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    const CppClassWizardDialog *wizard = qobject_cast<const CppClassWizardDialog *>(w);
    const CppClassWizardParameters params = wizard->parameters();

    const QString sourceFileName = Core::BaseFileWizard::buildFileName(params.path, params.sourceFile, sourceSuffix());
    const QString headerFileName = Core::BaseFileWizard::buildFileName(params.path, params.headerFile, headerSuffix());

    Core::GeneratedFile sourceFile(sourceFileName);
    Core::GeneratedFile headerFile(headerFileName);

    QString header, source;
    if (!generateHeaderAndSource(params, &header, &source)) {
        *errorMessage = tr("Error while generating file contents.");
        return Core::GeneratedFiles();
    }
    headerFile.setContents(header);
    headerFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    sourceFile.setContents(source);
    sourceFile.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << headerFile << sourceFile;
}


bool CppClassWizard::generateHeaderAndSource(const CppClassWizardParameters &params,
                                             QString *header, QString *source)
{
    // TODO:
    //  Quite a bit of this code has been copied from FormClassWizardParameters::generateCpp
    //  and is duplicated in the library wizard.
    //  Maybe more of it could be merged into Utils.

    const QString indent = QString(4, QLatin1Char(' '));

    // Do we have namespaces?
    QStringList namespaceList = params.className.split(QLatin1String("::"));
    if (namespaceList.empty()) // Paranoia!
        return false;

    const QString headerLicense =
            CppTools::AbstractEditorSupport::licenseTemplate(params.headerFile,
                                                             params.className);
    const QString sourceLicense =
            CppTools::AbstractEditorSupport::licenseTemplate(params.sourceFile,
                                                             params.className);

    const QString unqualifiedClassName = namespaceList.takeLast();
    const QString guard = Utils::headerGuard(params.headerFile, namespaceList);

    // == Header file ==
    QTextStream headerStr(header);
    headerStr << headerLicense << "#ifndef " << guard
              << "\n#define " <<  guard << '\n';

    QRegExp qtClassExpr(QLatin1String("^Q[A-Z3].+"));
    QTC_CHECK(qtClassExpr.isValid());
    // Determine parent QObject type for Qt types. Provide base
    // class in case the user did not specify one.
    QString parentQObjectClass;
    bool defineQObjectMacro = false;
    switch (params.classType) {
    case Utils::NewClassWidget::ClassInheritsQObject:
        parentQObjectClass = QLatin1String("QObject");
        defineQObjectMacro = true;
        break;
    case Utils::NewClassWidget::ClassInheritsQWidget:
        parentQObjectClass = QLatin1String("QWidget");
        defineQObjectMacro = true;
        break;
    case Utils::NewClassWidget::ClassInheritsQDeclarativeItem:
        parentQObjectClass = QLatin1String("QDeclarativeItem");
        defineQObjectMacro = true;
        break;
    case Utils::NewClassWidget::ClassInheritsQQuickItem:
        parentQObjectClass = QLatin1String("QQuickItem");
        defineQObjectMacro = true;
        break;
    case Utils::NewClassWidget::NoClassType:
    case Utils::NewClassWidget::SharedDataClass:
        break;
    }
    const QString baseClass = params.baseClass.isEmpty()
                              && params.classType != Utils::NewClassWidget::NoClassType ?
                              parentQObjectClass : params.baseClass;
    const bool superIsQtClass = qtClassExpr.exactMatch(baseClass);
    if (superIsQtClass) {
        headerStr << '\n';
        Utils::writeIncludeFileDirective(baseClass, true, headerStr);
    }
    if (params.classType == Utils::NewClassWidget::SharedDataClass) {
        headerStr << '\n';
        Utils::writeIncludeFileDirective(QLatin1String("QSharedDataPointer"), true, headerStr);
    }

    const QString namespaceIndent = Utils::writeOpeningNameSpaces(namespaceList, QString(), headerStr);

    const QString sharedDataClass = unqualifiedClassName + QLatin1String("Data");

    if (params.classType == Utils::NewClassWidget::SharedDataClass)
        headerStr << '\n' << "class " << sharedDataClass << ";\n";

    // Class declaration
    headerStr << '\n' << namespaceIndent << "class " << unqualifiedClassName;
    if (!baseClass.isEmpty())
        headerStr << " : public " << baseClass << "\n";
    else
        headerStr << "\n";
    headerStr << namespaceIndent << "{\n";
    if (defineQObjectMacro)
        headerStr << namespaceIndent << indent << "Q_OBJECT\n";
    headerStr << namespaceIndent << "public:\n"
              << namespaceIndent << indent;
    // Constructor
    if (parentQObjectClass.isEmpty()) {
        headerStr << unqualifiedClassName << "();\n";
    } else {
        headerStr << "explicit " << unqualifiedClassName << '(' << parentQObjectClass
                << " *parent = 0);\n";
    }
    // Copy/Assignment for shared data classes.
    if (params.classType == Utils::NewClassWidget::SharedDataClass) {
        headerStr << namespaceIndent << indent
                  << unqualifiedClassName << "(const " << unqualifiedClassName << " &);\n"
                  << namespaceIndent << indent
                  << unqualifiedClassName << " &operator=(const " << unqualifiedClassName << " &);\n"
                  << namespaceIndent << indent
                  << '~' << unqualifiedClassName << "();\n";
    }
    if (defineQObjectMacro)
        headerStr << '\n' << namespaceIndent << "signals:\n\n" << namespaceIndent << "public slots:\n\n";
    if (params.classType == Utils::NewClassWidget::SharedDataClass) {
        headerStr << '\n' << namespaceIndent << "private:\n"
                  << namespaceIndent << indent << "QSharedDataPointer<" << sharedDataClass << "> data;\n";
    }
    headerStr << namespaceIndent << "};\n";

    Utils::writeClosingNameSpaces(namespaceList, QString(), headerStr);

    headerStr << '\n';
    headerStr << "#endif // "<<  guard << '\n';

    // == Source file ==
    QTextStream sourceStr(source);
    sourceStr << sourceLicense;
    Utils::writeIncludeFileDirective(params.headerFile, false, sourceStr);
    if (params.classType == Utils::NewClassWidget::SharedDataClass)
        Utils::writeIncludeFileDirective(QLatin1String("QSharedData"), true, sourceStr);

    Utils::writeOpeningNameSpaces(namespaceList, QString(), sourceStr);
    // Private class:
    if (params.classType == Utils::NewClassWidget::SharedDataClass) {
        sourceStr << '\n' << namespaceIndent << "class " << sharedDataClass
                  << " : public QSharedData {\n"
                  << namespaceIndent << "public:\n"
                  << namespaceIndent << "};\n";
    }

    // Constructor
    sourceStr << '\n' << namespaceIndent;
    if (parentQObjectClass.isEmpty()) {
        sourceStr << unqualifiedClassName << "::" << unqualifiedClassName << "()";
        if (params.classType == Utils::NewClassWidget::SharedDataClass)
            sourceStr << " : data(new " << sharedDataClass << ')';
        sourceStr << '\n';
    } else {
        sourceStr << unqualifiedClassName << "::" << unqualifiedClassName
                << '(' << parentQObjectClass << " *parent) :\n"
                << namespaceIndent << indent << baseClass << "(parent)\n";
    }

    sourceStr << namespaceIndent << "{\n" << namespaceIndent << "}\n";
    if (params.classType == Utils::NewClassWidget::SharedDataClass) {
        // Copy
        sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName << "(const "
                << unqualifiedClassName << " &rhs) : data(rhs.data)\n"
                << namespaceIndent << "{\n" << namespaceIndent << "}\n\n";
        // Assignment
        sourceStr << namespaceIndent << unqualifiedClassName << " &"
                  << unqualifiedClassName << "::operator=(const " << unqualifiedClassName << " &rhs)\n"
                  << namespaceIndent << "{\n"
                  << namespaceIndent << indent << "if (this != &rhs)\n"
                  << namespaceIndent << indent << indent << "data.operator=(rhs.data);\n"
                  << namespaceIndent << indent << "return *this;\n"
                  << namespaceIndent << "}\n\n";
         // Destructor
        sourceStr << namespaceIndent << unqualifiedClassName << "::~"
                  << unqualifiedClassName << "()\n"
                  << namespaceIndent << "{\n"
                  << namespaceIndent << "}\n";
    }
    Utils::writeClosingNameSpaces(namespaceList, QString(), sourceStr);
    return true;
}
