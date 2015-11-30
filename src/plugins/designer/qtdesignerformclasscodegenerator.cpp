/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qtdesignerformclasscodegenerator.h"
#include "formtemplatewizardpage.h"
#include <designer/cpp/formclasswizardparameters.h>

#include <utils/codegeneration.h>
#include <coreplugin/icore.h>
#include <cpptools/abstracteditorsupport.h>
#include <qtsupport/codegenerator.h>
#include <qtsupport/codegensettings.h>

#include <QTextStream>
#include <QSettings>
#include <QFileInfo>
#include <QDebug>

static const char uiMemberC[] = "ui";
static const char uiNamespaceC[] = "Ui";

namespace Designer {
namespace Internal {

// Generation code

// Write out how to access the Ui class in the source code.
static inline void writeUiMemberAccess(const QtSupport::CodeGenSettings &fp, QTextStream &str)
{
    switch (fp.embedding) {
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

} // namespace Internal

bool QtDesignerFormClassCodeGenerator::generateCpp(const FormClassWizardParameters &parameters,
                                                   QString *header, QString *source, int indentation)
{
    QtSupport::CodeGenSettings generationParameters;
    generationParameters.fromSettings(Core::ICore::settings());

    const QString indent = QString(indentation, QLatin1Char(' '));
    QString formBaseClass;
    QString uiClassName;

    if (!QtSupport::CodeGenerator::uiData(parameters.uiTemplate, &formBaseClass, &uiClassName)) {
        qWarning("Unable to determine the form base class from %s.", qPrintable(parameters.uiTemplate));
        return false;
    }

    // Build the ui class (Ui::Foo) name relative to the namespace (which is the same):
    const QString colonColon = QLatin1String("::");
    const int lastSeparator = uiClassName.lastIndexOf(colonColon);
    if (lastSeparator != -1)
        uiClassName.remove(0, lastSeparator + colonColon.size());
    uiClassName.insert(0, QLatin1String(uiNamespaceC) + colonColon);

    // Do we have namespaces?
    QStringList namespaceList = parameters.className.split(colonColon);
    if (namespaceList.empty()) // Paranoia!
        return false;

    const QString unqualifiedClassName = namespaceList.takeLast();

    const QString headerLicense =
            CppTools::AbstractEditorSupport::licenseTemplate(parameters.headerFile, parameters.className);
    const QString sourceLicense =
            CppTools::AbstractEditorSupport::licenseTemplate(parameters.sourceFile, parameters.className);
    // Include guards
    const QString guard = Utils::headerGuard(parameters.headerFile, namespaceList);

    QString uiInclude = QLatin1String("ui_");
    uiInclude += QFileInfo(parameters.uiFile).completeBaseName();
    uiInclude += QLatin1String(".h");

    // 1) Header file
    QTextStream headerStr(header);
    headerStr << headerLicense << "#ifndef " << guard
              << "\n#define " <<  guard << '\n' << '\n';

    // Include 'ui_'
    if (generationParameters.embedding != QtSupport::CodeGenSettings::PointerAggregatedUiClass) {
        Utils::writeIncludeFileDirective(uiInclude, false, headerStr);
    } else {
        // Todo: Can we obtain the header from the code model for custom widgets?
        // Alternatively, from Designer.
        if (formBaseClass.startsWith(QLatin1Char('Q'))) {
            if (generationParameters.includeQtModule) {
                if (generationParameters.addQtVersionCheck) {
                    Utils::writeBeginQtVersionCheck(headerStr);
                    Utils::writeIncludeFileDirective(QLatin1String("QtWidgets/") + formBaseClass, true, headerStr);
                    headerStr << "#else\n";
                    Utils::writeIncludeFileDirective(QLatin1String("QtGui/") + formBaseClass, true, headerStr);
                    headerStr << "#endif\n";
                } else {
                    Utils::writeIncludeFileDirective(QLatin1String("QtGui/") + formBaseClass, true, headerStr);
                }
            } else {
                Utils::writeIncludeFileDirective(formBaseClass, true, headerStr);
            }
        }
    }

    const QString namespaceIndent = Utils::writeOpeningNameSpaces(namespaceList, QString(),
                                                                  headerStr);

    // Forward-declare the UI class
    if (generationParameters.embedding == QtSupport::CodeGenSettings::PointerAggregatedUiClass) {
          headerStr << '\n'
                  << namespaceIndent << "namespace " <<  uiNamespaceC << " {\n"
                  << namespaceIndent << indent << "class " << Internal::FormTemplateWizardPage::stripNamespaces(uiClassName) << ";\n"
                  << namespaceIndent << "}\n";
    }

    // Class declaration
    headerStr << '\n' << namespaceIndent << "class " << unqualifiedClassName
              << " : public " << formBaseClass;
    if (generationParameters.embedding == QtSupport::CodeGenSettings::InheritedUiClass)
        headerStr << ", private " << uiClassName;
    headerStr << "\n{\n" << namespaceIndent << indent << "Q_OBJECT\n\n"
              << namespaceIndent << "public:\n"
              << namespaceIndent << indent << "explicit " << unqualifiedClassName << "(QWidget *parent = 0);\n";
    if (generationParameters.embedding == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
        headerStr << namespaceIndent << indent << "~" << unqualifiedClassName << "();\n";
    // retranslation
    if (generationParameters.retranslationSupport)
        headerStr << '\n' << namespaceIndent << "protected:\n"
                  << namespaceIndent << indent << "void changeEvent(QEvent *e);\n";
    // Member variable
    if (generationParameters.embedding != QtSupport::CodeGenSettings::InheritedUiClass) {
        headerStr << '\n' << namespaceIndent << "private:\n"
                  << namespaceIndent << indent << uiClassName << ' ';
        if (generationParameters.embedding == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
            headerStr << '*';
        headerStr << uiMemberC << ";\n";
    }
    headerStr << namespaceIndent << "};\n\n";
    Utils::writeClosingNameSpaces(namespaceList, QString(), headerStr);
    headerStr << "#endif // "<<  guard << '\n';

    // 2) Source file
    QTextStream sourceStr(source);
    sourceStr << sourceLicense;
    Utils::writeIncludeFileDirective(parameters.headerFile, false, sourceStr);
    if (generationParameters.embedding == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
        Utils::writeIncludeFileDirective(uiInclude, false, sourceStr);
    // NameSpaces(
    Utils::writeOpeningNameSpaces(namespaceList, QString(), sourceStr);
    // Constructor with setupUi
    sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName << "(QWidget *parent) :\n"
               << namespaceIndent << indent << formBaseClass << "(parent)";
    if (generationParameters.embedding == QtSupport::CodeGenSettings::PointerAggregatedUiClass)
        sourceStr << ",\n"  << namespaceIndent << indent <<  uiMemberC << "(new " << uiClassName << ")";
    sourceStr <<  '\n' << namespaceIndent << "{\n" <<  namespaceIndent << indent;
    Internal::writeUiMemberAccess(generationParameters, sourceStr);
    sourceStr <<  "setupUi(this);\n" << namespaceIndent << "}\n";
    // Deleting destructor for ptr
    if (generationParameters.embedding == QtSupport::CodeGenSettings::PointerAggregatedUiClass) {
        sourceStr << '\n' <<  namespaceIndent << unqualifiedClassName << "::~" << unqualifiedClassName
                  << "()\n" << namespaceIndent << "{\n"
                  << namespaceIndent << indent << "delete " << uiMemberC << ";\n"
                  << namespaceIndent << "}\n";
    }
    // retranslation
    if (generationParameters.retranslationSupport) {
        sourceStr  << '\n' << namespaceIndent << "void " << unqualifiedClassName << "::" << "changeEvent(QEvent *e)\n"
        << namespaceIndent << "{\n"
        << namespaceIndent << indent << formBaseClass << "::changeEvent(e);\n"
        << namespaceIndent << indent << "switch (e->type()) {\n" << namespaceIndent << indent << "case QEvent::LanguageChange:\n"
        << namespaceIndent << indent << indent;
        Internal::writeUiMemberAccess(generationParameters, sourceStr);
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

QtDesignerFormClassCodeGenerator::QtDesignerFormClassCodeGenerator(QObject *parent) :
    QObject(parent)
{
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
