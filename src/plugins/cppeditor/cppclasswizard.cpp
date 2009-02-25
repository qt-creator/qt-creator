/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cppclasswizard.h"
#include "cppeditorconstants.h"

#include <utils/codegeneration.h>
#include <utils/newclasswidget.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTextStream>

#include <QtGui/QVBoxLayout>
#include <QtGui/QWizard>

using namespace CppEditor;
using namespace CppEditor::Internal;


// ========= ClassNamePage =========

ClassNamePage::ClassNamePage(const QString &sourceSuffix,
                             const QString &headerSuffix,
                             QWidget *parent) :
    QWizardPage(parent),
    m_isValid(false)
{
    setTitle(tr("Enter class name"));
    setSubTitle(tr("The header and source file names will be derived from the class name"));

    m_newClassWidget = new Core::Utils::NewClassWidget;
    // Order, set extensions first before suggested name is derived
    m_newClassWidget->setHeaderExtension(headerSuffix);
    m_newClassWidget->setSourceExtension(sourceSuffix);
    m_newClassWidget->setBaseClassInputVisible(true);
    m_newClassWidget->setBaseClassChoices(QStringList() << QString()
            << QLatin1String("QObject")
            << QLatin1String("QWidget")
            << QLatin1String("QMainWindow"));
    m_newClassWidget->setBaseClassEditable(true);
    m_newClassWidget->setFormInputVisible(false);
    m_newClassWidget->setNamespacesEnabled(true);
    m_newClassWidget->setAllowDirectories(true);

    connect(m_newClassWidget, SIGNAL(validChanged()),
            this, SLOT(slotValidChanged()));

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->addWidget(m_newClassWidget);
}

void ClassNamePage::slotValidChanged()
{
    const bool validNow = m_newClassWidget->isValid();
    if (m_isValid != validNow) {
        m_isValid = validNow;
        emit completeChanged();
    }
}

CppClassWizardDialog::CppClassWizardDialog(const QString &sourceSuffix,
                                           const QString &headerSuffix,
                                           QWidget *parent) :
    QWizard(parent),
    m_classNamePage(new ClassNamePage(sourceSuffix, headerSuffix, this))
{
    Core::BaseFileWizard::setupWizard(this);
    setWindowTitle(tr("C++ Class Wizard"));
    addPage(m_classNamePage);
}

void CppClassWizardDialog::setPath(const QString &path)
{
    m_classNamePage->newClassWidget()->setPath(path);
}

CppClassWizardParameters  CppClassWizardDialog::parameters() const
{
    CppClassWizardParameters rc;
    const Core::Utils::NewClassWidget *ncw = m_classNamePage->newClassWidget();
    rc.className = ncw->className();
    rc.headerFile = ncw->headerFileName();
    rc.sourceFile = ncw->sourceFileName();
    rc.baseClass = ncw->baseClassName();
    rc.path = ncw->path();
    return rc;
}

// ========= CppClassWizard =========

CppClassWizard::CppClassWizard(const Core::BaseFileWizardParameters &parameters,
                               QObject *parent)
  : Core::BaseFileWizard(parameters, parent)
{
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
                                            const QString &defaultPath,
                                            const WizardPageList &extensionPages) const
{
    CppClassWizardDialog *wizard = new CppClassWizardDialog(sourceSuffix(), headerSuffix(), parent);
    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);
    wizard->setPath(defaultPath);
    return wizard;
}

Core::GeneratedFiles CppClassWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    const CppClassWizardDialog *wizard = qobject_cast<const CppClassWizardDialog *>(w);
    const CppClassWizardParameters params = wizard->parameters();

    const QString sourceFileName = Core::BaseFileWizard::buildFileName(params.path, params.sourceFile, sourceSuffix());
    const QString headerFileName = Core::BaseFileWizard::buildFileName(params.path, params.headerFile, headerSuffix());

    Core::GeneratedFile sourceFile(sourceFileName);
    sourceFile.setEditorKind(QLatin1String(Constants::CPPEDITOR_KIND));

    Core::GeneratedFile headerFile(headerFileName);
    headerFile.setEditorKind(QLatin1String(Constants::CPPEDITOR_KIND));

    QString header, source;
    if (!generateHeaderAndSource(params, &header, &source)) {
        *errorMessage = tr("Error while generating file contents.");
        return Core::GeneratedFiles();
    }
    headerFile.setContents(header);
    sourceFile.setContents(source);
    return Core::GeneratedFiles() << headerFile << sourceFile;
}


bool CppClassWizard::generateHeaderAndSource(const CppClassWizardParameters &params,
                                             QString *header, QString *source)
{
    // TODO:
    //  Quite a bit of this code has been copied from FormClassWizardParameters::generateCpp.
    //  Maybe more of it could be merged into Core::Utils.

    const QString indent = QString(4, QLatin1Char(' '));

    // Do we have namespaces?
    QStringList namespaceList = params.className.split(QLatin1String("::"));
    if (namespaceList.empty()) // Paranoia!
        return false;

    const QString unqualifiedClassName = namespaceList.takeLast();
    const QString guard = Core::Utils::headerGuard(unqualifiedClassName);

    // == Header file ==
    QTextStream headerStr(header);
    headerStr << "#ifndef " << guard
              << "\n#define " <<  guard << '\n' << '\n';

    const QRegExp qtClassExpr(QLatin1String("^Q[A-Z3].+"));
    QTC_ASSERT(qtClassExpr.isValid(), /**/);
    const bool superIsQtClass = qtClassExpr.exactMatch(params.baseClass);
    if (superIsQtClass) {
        Core::Utils::writeIncludeFileDirective(params.baseClass, true, headerStr);
        headerStr << '\n';
    }

    const QString namespaceIndent = Core::Utils::writeOpeningNameSpaces(namespaceList, 0, headerStr);

    // Class declaration
    headerStr << namespaceIndent << "class " << unqualifiedClassName;
    if (!params.baseClass.isEmpty())
        headerStr << " : public " << params.baseClass << "\n";
    else
        headerStr << "\n";
    headerStr << namespaceIndent << "{\n";
    headerStr << namespaceIndent << "public:\n"
              << namespaceIndent << indent << unqualifiedClassName << "();\n";
    headerStr << namespaceIndent << "};\n\n";

    Core::Utils::writeClosingNameSpaces(namespaceList, 0, headerStr);
    headerStr << "#endif // "<<  guard << '\n';


    // == Source file ==
    QTextStream sourceStr(source);
    Core::Utils::writeIncludeFileDirective(params.headerFile, false, sourceStr);
    Core::Utils::writeOpeningNameSpaces(namespaceList, 0, sourceStr);

    // Constructor
    sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName << "()\n";
    sourceStr << namespaceIndent << "{\n" << namespaceIndent << "}\n";

    Core::Utils::writeClosingNameSpaces(namespaceList, indent, sourceStr);
    return true;
}
