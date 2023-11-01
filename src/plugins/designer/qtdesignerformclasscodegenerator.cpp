// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtdesignerformclasscodegenerator.h"
#include "formtemplatewizardpage.h"
#include <designer/cpp/formclasswizardparameters.h>

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <cppeditor/abstracteditorsupport.h>
#include <projectexplorer/projecttree.h>
#include <qtsupport/codegenerator.h>
#include <qtsupport/codegensettings.h>
#include <utils/codegeneration.h>

#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

using namespace Utils;

static const char uiMemberC[] = "ui";
static const char uiNamespaceC[] = "Ui";

namespace Designer {

// Generation code

// Write out how to access the Ui class in the source code.
static void writeUiMemberAccess(const QtSupport::CodeGenSettings &fp, QTextStream &str)
{
    switch (fp.embedding()) {
    case QtSupport::CodeGenSettings::PointerAggregatedUiClass:
        str << uiMemberC << "->";
        break;
    case QtSupport::CodeGenSettings::AggregatedUiClass:
        str << uiMemberC << '.';
        break;
    case QtSupport::CodeGenSettings::InheritedUiClass:
        break;
    }
}

bool QtDesignerFormClassCodeGenerator::generateCpp(const FormClassWizardParameters &parameters,
                                                   QString *header, QString *source, int indentation)
{
    const QtSupport::CodeGenSettings &generationParameters = QtSupport::codeGenSettings();

    const QString indent = QString(indentation, ' ');
    QString formBaseClass;
    QString uiClassName;

    if (!QtSupport::CodeGenerator::uiData(parameters.uiTemplate, &formBaseClass, &uiClassName)) {
        qWarning("Unable to determine the form base class from %s.", qPrintable(parameters.uiTemplate));
        return false;
    }

    // Build the ui class (Ui::Foo) name relative to the namespace (which is the same):
    const QString colonColon = "::";
    const int lastSeparator = uiClassName.lastIndexOf(colonColon);
    if (lastSeparator != -1)
        uiClassName.remove(0, lastSeparator + colonColon.size());
    uiClassName.insert(0, uiNamespaceC + colonColon);

    // Do we have namespaces?
    QStringList namespaceList = parameters.className.split(colonColon);
    if (namespaceList.empty()) // Paranoia!
        return false;

    const QString unqualifiedClassName = namespaceList.takeLast();

    ProjectExplorer::Project * const project = ProjectExplorer::ProjectTree::currentProject();
    const QString headerLicense = CppEditor::AbstractEditorSupport::licenseTemplate(
        project, FilePath::fromString(parameters.headerFile), parameters.className);
    const QString sourceLicense = CppEditor::AbstractEditorSupport::licenseTemplate(
        project, FilePath::fromString(parameters.sourceFile), parameters.className);
    // Include guards
    const QString guard = Utils::headerGuard(parameters.headerFile, namespaceList);

    const QString uiInclude = "ui_" + QFileInfo(parameters.uiFile).completeBaseName() + ".h";

    // 1) Header file
    QTextStream headerStr(header);
    headerStr << headerLicense;

    if (parameters.usePragmaOnce)
        headerStr << "#pragma once\n\n";
    else
        headerStr << "#ifndef " << guard << "\n#define " << guard << "\n\n";

    // Include 'ui_'
    if (generationParameters.embedding() != QtSupport::CodeGenSettings::PointerAggregatedUiClass) {
        Utils::writeIncludeFileDirective(uiInclude, false, headerStr);
    } else {
        // Todo: Can we obtain the header from the code model for custom widgets?
        // Alternatively, from Designer.
        if (formBaseClass.startsWith('Q')) {
            if (generationParameters.includeQtModule()) {
                if (generationParameters.addQtVersionCheck()) {
                    Utils::writeBeginQtVersionCheck(headerStr);
                    Utils::writeIncludeFileDirective("QtWidgets/" + formBaseClass, true, headerStr);
                    headerStr << "#else\n";
                    Utils::writeIncludeFileDirective("QtGui/" + formBaseClass, true, headerStr);
                    headerStr << "#endif\n";
                } else {
                    Utils::writeIncludeFileDirective("QtGui/" + formBaseClass, true, headerStr);
                }
            } else {
                Utils::writeIncludeFileDirective(formBaseClass, true, headerStr);
            }
        }
    }

    const QString namespaceIndent = Utils::writeOpeningNameSpaces(namespaceList, QString(),
                                                                  headerStr);

    // Forward-declare the UI class
    if (generationParameters.embedding() == QtSupport::CodeGenSettings::PointerAggregatedUiClass) {
          headerStr << '\n'
                  << namespaceIndent << "namespace " <<  uiNamespaceC << " {\n"
                  << namespaceIndent << indent << "class " << Internal::FormTemplateWizardPage::stripNamespaces(uiClassName) << ";\n"
                  << namespaceIndent << "}\n";
    }

    // Class declaration
    headerStr << '\n' << namespaceIndent << "class " << unqualifiedClassName
              << " : public " << formBaseClass;
    if (generationParameters.embedding() == QtSupport::CodeGenSettings::InheritedUiClass)
        headerStr << ", private " << uiClassName;
    headerStr << "\n{\n" << namespaceIndent << indent << "Q_OBJECT\n\n"
              << namespaceIndent << "public:\n"
              << namespaceIndent << indent << "explicit " << unqualifiedClassName << "(QWidget *parent = nullptr);\n";
    if (generationParameters.embedding() == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
        headerStr << namespaceIndent << indent << "~" << unqualifiedClassName << "();\n";
    // retranslation
    if (generationParameters.retranslationSupport())
        headerStr << '\n' << namespaceIndent << "protected:\n"
                  << namespaceIndent << indent << "void changeEvent(QEvent *e);\n";
    // Member variable
    if (generationParameters.embedding() != QtSupport::CodeGenSettings::InheritedUiClass) {
        headerStr << '\n' << namespaceIndent << "private:\n"
                  << namespaceIndent << indent << uiClassName << ' ';
        if (generationParameters.embedding() == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
            headerStr << '*';
        headerStr << uiMemberC << ";\n";
    }
    headerStr << namespaceIndent << "};\n\n";
    Utils::writeClosingNameSpaces(namespaceList, QString(), headerStr);

    if (!parameters.usePragmaOnce)
        headerStr << "#endif // " << guard << '\n';

    // 2) Source file
    QTextStream sourceStr(source);
    sourceStr << sourceLicense;
    Utils::writeIncludeFileDirective(parameters.headerFile, false, sourceStr);
    if (generationParameters.embedding() == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
        Utils::writeIncludeFileDirective(uiInclude, false, sourceStr);
    // NameSpaces(
    Utils::writeOpeningNameSpaces(namespaceList, QString(), sourceStr);
    // Constructor with setupUi
    sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName << "(QWidget *parent) :\n"
               << namespaceIndent << indent << formBaseClass << "(parent)";
    if (generationParameters.embedding() == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
        sourceStr << ",\n"  << namespaceIndent << indent <<  uiMemberC << "(new " << uiClassName << ")";
    sourceStr <<  '\n' << namespaceIndent << "{\n" <<  namespaceIndent << indent;
    writeUiMemberAccess(generationParameters, sourceStr);
    sourceStr <<  "setupUi(this);\n" << namespaceIndent << "}\n";
    // Deleting destructor for ptr
    if (generationParameters.embedding() == QtSupport::CodeGenSettings::PointerAggregatedUiClass) {
        sourceStr << '\n' <<  namespaceIndent << unqualifiedClassName << "::~" << unqualifiedClassName
                  << "()\n" << namespaceIndent << "{\n"
                  << namespaceIndent << indent << "delete " << uiMemberC << ";\n"
                  << namespaceIndent << "}\n";
    }
    // retranslation
    if (generationParameters.retranslationSupport()) {
        sourceStr  << '\n' << namespaceIndent << "void " << unqualifiedClassName << "::" << "changeEvent(QEvent *e)\n"
        << namespaceIndent << "{\n"
        << namespaceIndent << indent << formBaseClass << "::changeEvent(e);\n"
        << namespaceIndent << indent << "switch (e->type()) {\n" << namespaceIndent << indent << "case QEvent::LanguageChange:\n"
        << namespaceIndent << indent << indent;
        writeUiMemberAccess(generationParameters, sourceStr);
        sourceStr << "retranslateUi(this);\n"
                  << namespaceIndent << indent <<  indent << "break;\n"
                  << namespaceIndent << indent << "default:\n"
                  << namespaceIndent << indent << indent << "break;\n"
                  << namespaceIndent << indent << "}\n"
                  << namespaceIndent << "}\n";
    }
    Utils::writeClosingNameSpaces(namespaceList, QString(), sourceStr);
    return true;
}

QtDesignerFormClassCodeGenerator::QtDesignerFormClassCodeGenerator()
{
    setObjectName("QtDesignerFormClassCodeGenerator");
    ExtensionSystem::PluginManager::addObject(this);
}

QtDesignerFormClassCodeGenerator::~QtDesignerFormClassCodeGenerator()
{
    ExtensionSystem::PluginManager::removeObject(this);
}

QVariant QtDesignerFormClassCodeGenerator::generateFormClassCode(const FormClassWizardParameters &parameters)
{
    QString header;
    QString source;
    QtDesignerFormClassCodeGenerator::generateCpp(parameters, &header, &source);
    QVariantList result;
    result << header << source;
    return QVariant(result);
}

} // namespace Designer
