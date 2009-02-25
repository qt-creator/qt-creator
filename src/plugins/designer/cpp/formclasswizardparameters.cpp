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

#include "formclasswizardparameters.h"
#include "formtemplatewizardpage.h"

#include <utils/codegeneration.h>

#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

static const char *uiMemberC = "m_ui";
static const char *uiNamespaceC = "Ui";

namespace Designer {
namespace Internal {

FormClassWizardParameters::FormClassWizardParameters() :
    embedding(PointerAggregatedUiClass),
    languageChange(true)
{
}

bool FormClassWizardParameters::generateCpp(QString *header, QString *source, int indentation) const
{
    const QString indent = QString(indentation, QLatin1Char(' '));
    QString formBaseClass;
    QString uiClassName;
    if (!FormTemplateWizardPage::getUIXmlData(uiTemplate, &formBaseClass, &uiClassName)) {
        qWarning("Unable to determine the form base class from %s.", uiTemplate.toUtf8().constData());
        return false;
    }

    // Build the ui class (Ui::Foo) name relative to the namespace (which is the same):
    const QString colonColon = QLatin1String("::");
    const int lastSeparator = uiClassName.lastIndexOf(colonColon);
    if (lastSeparator != -1)
        uiClassName.remove(0, lastSeparator + colonColon.size());
    uiClassName.insert(0, QLatin1String(uiNamespaceC) + colonColon);

    // Do we have namespaces?
    QStringList namespaceList = className.split(colonColon);
    if (namespaceList.empty()) // Paranoia!
        return false;

    const QString unqualifiedClassName = namespaceList.takeLast();

    // Include guards
    const QString guard = Core::Utils::headerGuard(unqualifiedClassName);

    QString uiInclude = QLatin1String("ui_");
    uiInclude += QFileInfo(uiFile).baseName();
    uiInclude += QLatin1String(".h");

    // 1) Header file
    QTextStream headerStr(header);
    headerStr << "#ifndef " << guard
              << "\n#define " <<  guard << '\n' << '\n';

    // Include 'ui_'
    if (embedding != PointerAggregatedUiClass) {
        Core::Utils::writeIncludeFileDirective(uiInclude, false, headerStr);
    } else {
        // Todo: Can we obtain the header from the code model for custom widgets?
        // Alternatively, from Designer.
        if (formBaseClass.startsWith(QLatin1Char('Q'))) {
            QString baseInclude = QLatin1String("QtGui/");
            baseInclude += formBaseClass;
            Core::Utils::writeIncludeFileDirective(baseInclude, true, headerStr);
        }
    }

    const QString namespaceIndent = Core::Utils::writeOpeningNameSpaces(namespaceList, indent, headerStr);

    // Forward-declare the UI class
    if (embedding == PointerAggregatedUiClass) {
          headerStr << '\n'
                  << namespaceIndent << "namespace " <<  uiNamespaceC << " {\n"
                  << namespaceIndent << indent << "class " << FormTemplateWizardPage::stripNamespaces(uiClassName) << ";\n"
                  << namespaceIndent << "}\n";
    }

    // Class declaration
    headerStr << '\n' << namespaceIndent << "class " << unqualifiedClassName
              << " : public " << formBaseClass;
    if (embedding == InheritedUiClass) {
        headerStr << ", private " << uiClassName;
    }
    headerStr << " {\n" << namespaceIndent << indent << "Q_OBJECT\n"
              << namespaceIndent << indent << "Q_DISABLE_COPY(" << unqualifiedClassName << ")\n"
              << namespaceIndent << "public:\n"
              << namespaceIndent << indent << "explicit " << unqualifiedClassName << "(QWidget *parent = 0);\n";
    if (embedding == PointerAggregatedUiClass)
        headerStr << namespaceIndent << indent << "virtual ~" << unqualifiedClassName << "();\n";
    // retranslation
    if (languageChange)
        headerStr << '\n' << namespaceIndent << "protected:\n"
                  << namespaceIndent << indent << "virtual void changeEvent(QEvent *e);\n";
    // Member variable
    if (embedding != InheritedUiClass) {
        headerStr << '\n' << namespaceIndent << "private:\n"
                  << namespaceIndent << indent << uiClassName << ' ';
        if (embedding == PointerAggregatedUiClass)
            headerStr << '*';
        headerStr << uiMemberC << ";\n";
    }
    headerStr << namespaceIndent << "};\n\n";
    Core::Utils::writeClosingNameSpaces(namespaceList, indent, headerStr);
    headerStr << "#endif // "<<  guard << '\n';

    // 2) Source file
    QTextStream sourceStr(source);
    Core::Utils::writeIncludeFileDirective(headerFile, false, sourceStr);
    if (embedding == PointerAggregatedUiClass)
        Core::Utils::writeIncludeFileDirective(uiInclude, false, sourceStr);
    // NameSpaces(
    Core::Utils::writeOpeningNameSpaces(namespaceList, indent, sourceStr);
    // Constructor with setupUi
    sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName << "(QWidget *parent) :\n"
               << namespaceIndent << indent << formBaseClass << "(parent)";
    if (embedding == PointerAggregatedUiClass)
        sourceStr << ",\n"  << namespaceIndent << indent <<  uiMemberC << "(new " << uiClassName << ")\n";
    sourceStr <<  namespaceIndent << "{\n" <<  namespaceIndent << indent;
    if (embedding != InheritedUiClass)
        sourceStr << uiMemberC << (embedding == PointerAggregatedUiClass ? "->" : ".");
    sourceStr << "setupUi(this);\n" << namespaceIndent << "}\n";
    // Deleting destructor for ptr
    if (embedding == PointerAggregatedUiClass) {
        sourceStr << '\n' <<  namespaceIndent << unqualifiedClassName << "::~" << unqualifiedClassName
                  << "()\n" << namespaceIndent << "{\n"
                  << namespaceIndent << indent << "delete " << uiMemberC << ";\n"
                  << namespaceIndent << "}\n";
    }
    // retranslation
    if (languageChange) {
        sourceStr  << '\n' << namespaceIndent << "void " << unqualifiedClassName << "::" << "changeEvent(QEvent *e)\n"
        << namespaceIndent << "{\n"
        << namespaceIndent << indent << "switch (e->type()) {\n" << namespaceIndent << indent << "case QEvent::LanguageChange:\n"
        << namespaceIndent << indent << indent;
        if (embedding != InheritedUiClass)
            sourceStr << uiMemberC << (embedding == PointerAggregatedUiClass ? "->" : ".");
        sourceStr << "retranslateUi(this);\n"
                  << namespaceIndent << indent <<  indent << "break;\n"
                  << namespaceIndent << indent << "default:\n"
                  << namespaceIndent << indent << indent << "break;\n"
                  << namespaceIndent << indent << "}\n"
                  << namespaceIndent << "}\n";
    }
    Core::Utils::writeClosingNameSpaces(namespaceList, indent, sourceStr);
    return true;
}

} // namespace Internal
} // namespace Designer
