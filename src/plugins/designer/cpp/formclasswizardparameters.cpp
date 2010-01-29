/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "formclasswizardparameters.h"
#include "formtemplatewizardpage.h"

#include <utils/codegeneration.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QSharedData>

static const char *uiMemberC = "ui";
static const char *uiNamespaceC = "Ui";

static const char *formClassWizardPageGroupC = "FormClassWizardPage";
static const char *translationKeyC = "RetranslationSupport";
static const char *embeddingModeKeyC = "Embedding";

// TODO: These 2 are general coding convention settings and
// should go to CppTools...
static const char *includeQtModuleKeyC = "IncludeQtModule";
static const char *indentNamespaceKeyC = "IndentNamespace";

namespace Designer {

class FormClassWizardGenerationParametersPrivate : public QSharedData
{
public:
    FormClassWizardGenerationParametersPrivate();
    void fromSettings(const QSettings *);
    void toSettings(QSettings *) const;
    bool equals(const FormClassWizardGenerationParametersPrivate &rhs) const;

    FormClassWizardGenerationParameters::UiClassEmbedding embedding;
    bool retranslationSupport; // Add handling for language change events
    bool includeQtModule;      // Include "<QtGui/[Class]>" or just "<[Class]>"
    bool indentNamespace;
};

FormClassWizardGenerationParametersPrivate::FormClassWizardGenerationParametersPrivate() :
    embedding(FormClassWizardGenerationParameters::PointerAggregatedUiClass),
    retranslationSupport(true),
    includeQtModule(false),
    indentNamespace(false)
{
}

void FormClassWizardGenerationParametersPrivate::fromSettings(const QSettings *settings)
{
    QString key = QLatin1String(formClassWizardPageGroupC);
    key += QLatin1Char('/');
    const int groupLength = key.size();

    key += QLatin1String(translationKeyC);
    retranslationSupport = settings->value(key, true).toBool();

    key.truncate(groupLength);
    key += QLatin1String(embeddingModeKeyC);
    embedding =  static_cast<FormClassWizardGenerationParameters::UiClassEmbedding>(settings->value(key, int(FormClassWizardGenerationParameters::PointerAggregatedUiClass)).toInt());

    key.truncate(groupLength);
    key += QLatin1String(includeQtModuleKeyC);
    includeQtModule = settings->value(key, false).toBool();

    key.truncate(groupLength);
    key += QLatin1String(indentNamespaceKeyC);
    indentNamespace = settings->value(key, false).toBool();
}

void FormClassWizardGenerationParametersPrivate::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(formClassWizardPageGroupC));
    settings->setValue(QLatin1String(translationKeyC), retranslationSupport);
    settings->setValue(QLatin1String(embeddingModeKeyC), embedding);
    settings->setValue(QLatin1String(includeQtModuleKeyC), includeQtModule);
    settings->setValue(QLatin1String(indentNamespaceKeyC), indentNamespace);
    settings->endGroup();
}

bool FormClassWizardGenerationParametersPrivate::equals(const FormClassWizardGenerationParametersPrivate &rhs) const
{
    return embedding == rhs.embedding && retranslationSupport == rhs.retranslationSupport
         && includeQtModule == rhs.includeQtModule && indentNamespace == rhs.indentNamespace;
}

FormClassWizardGenerationParameters::FormClassWizardGenerationParameters() :
    m_d(new FormClassWizardGenerationParametersPrivate)
{
}

FormClassWizardGenerationParameters::~FormClassWizardGenerationParameters()
{
}

FormClassWizardGenerationParameters::FormClassWizardGenerationParameters(const FormClassWizardGenerationParameters &rhs) :
    m_d(rhs.m_d)
{
}

FormClassWizardGenerationParameters &FormClassWizardGenerationParameters::operator=(const FormClassWizardGenerationParameters &rhs)
{
    if (this != &rhs)
        m_d.operator=(rhs.m_d);
    return *this;
}

bool FormClassWizardGenerationParameters::equals(const FormClassWizardGenerationParameters &rhs) const
{
    return m_d->equals(*rhs.m_d.constData());
}

FormClassWizardGenerationParameters::UiClassEmbedding FormClassWizardGenerationParameters::embedding() const
{
    return m_d->embedding;
}

void FormClassWizardGenerationParameters::setEmbedding(UiClassEmbedding e)
{
    m_d->embedding = e;
}

bool FormClassWizardGenerationParameters::retranslationSupport() const
{
    return m_d->retranslationSupport;
}

void FormClassWizardGenerationParameters::setRetranslationSupport(bool v)
{
     m_d->retranslationSupport = v;
}

bool FormClassWizardGenerationParameters::includeQtModule() const
{
    return m_d->includeQtModule;
}

void FormClassWizardGenerationParameters::setIncludeQtModule(bool v)
{
     m_d->includeQtModule = v;
}

bool FormClassWizardGenerationParameters::indentNamespace() const
{
    return m_d->indentNamespace;
}

void FormClassWizardGenerationParameters::setIndentNamespace(bool v)
{
     m_d->indentNamespace = v;
}

void FormClassWizardGenerationParameters::fromSettings(const QSettings *settings)
{
    m_d->fromSettings(settings);
}

void FormClassWizardGenerationParameters::toSettings(QSettings *settings) const
{
    m_d->toSettings(settings);
}

// -----------

class FormClassWizardParametersPrivate : public QSharedData {
public:
    bool generateCpp(const FormClassWizardGenerationParameters &fgp,
                     QString *header, QString *source, int indentation) const;

    QString uiTemplate;
    QString className;

    QString path;
    QString sourceFile;
    QString headerFile;
    QString uiFile;
};

FormClassWizardParameters::FormClassWizardParameters() :
    m_d(new FormClassWizardParametersPrivate)
{
}

FormClassWizardParameters::~FormClassWizardParameters()
{
}

FormClassWizardParameters::FormClassWizardParameters(const FormClassWizardParameters &rhs) :
        m_d(rhs.m_d)
{
}

FormClassWizardParameters &FormClassWizardParameters::operator=(const FormClassWizardParameters &rhs)
{
    if (this != &rhs)
        m_d.operator =(rhs.m_d);
    return *this;
}

QString FormClassWizardParameters::uiTemplate() const
{
    return m_d->uiTemplate;
}

void FormClassWizardParameters::setUiTemplate(const QString &s)
{
     m_d->uiTemplate = s;
}

QString FormClassWizardParameters::className() const
{
    return m_d->className;
}

void FormClassWizardParameters::setClassName(const QString &s)
{
     m_d->className = s;
}

QString FormClassWizardParameters::path() const
{
    return m_d->path;
}

void FormClassWizardParameters::setPath(const QString &s)
{
     m_d->path = s;
}


QString FormClassWizardParameters::sourceFile() const
{
    return m_d->sourceFile;
}

void FormClassWizardParameters::setSourceFile(const QString &s)
{
     m_d->sourceFile = s;
}

QString FormClassWizardParameters::headerFile() const
{
    return m_d->headerFile;
}

void FormClassWizardParameters::setHeaderFile(const QString &s)
{
     m_d->headerFile = s;
}


QString FormClassWizardParameters::uiFile() const
{
    return m_d->uiFile;
}

void FormClassWizardParameters::setUiFile(const QString &s)
{
     m_d->uiFile = s;
}

bool FormClassWizardParameters::getUIXmlData(const QString &uiXml, QString *formBaseClass, QString *uiClassName)
{
    return Designer::Internal::FormTemplateWizardPage::getUIXmlData(uiXml, formBaseClass, uiClassName);
}

QString FormClassWizardParameters::changeUiClassName(const QString &uiXml, const QString &newUiClassName)
{
    return Designer::Internal::FormTemplateWizardPage::changeUiClassName(uiXml, newUiClassName);
}

// Write out how to access the Ui class in the source code.
static inline void writeUiMemberAccess(const FormClassWizardGenerationParameters &fp, QTextStream &str)
{
    switch(fp.embedding()) {
    case FormClassWizardGenerationParameters::PointerAggregatedUiClass:
        str << uiMemberC << "->";
        break;
    case FormClassWizardGenerationParameters::AggregatedUiClass:
        str << uiMemberC << '.';
        break;
    case FormClassWizardGenerationParameters::InheritedUiClass:
        break;
    }
}

bool FormClassWizardParametersPrivate::generateCpp(const FormClassWizardGenerationParameters &generationParameters,
                                                   QString *header, QString *source, int indentation) const
{
    const QString indent = QString(indentation, QLatin1Char(' '));
    QString formBaseClass;
    QString uiClassName;
    if (!FormClassWizardParameters::getUIXmlData(uiTemplate, &formBaseClass, &uiClassName)) {
        qWarning("Unable to determine the form base class from %s.", uiTemplate.toUtf8().constData());
        return false;
    }

    // Build the ui class (Ui::Foo) name relative to the namespace (which is the same):
    const FormClassWizardGenerationParameters::UiClassEmbedding embedding = generationParameters.embedding();
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

    const QString license = CppTools::AbstractEditorSupport::licenseTemplate();
    // Include guards
    const QString guard = Utils::headerGuard(headerFile);

    QString uiInclude = QLatin1String("ui_");
    uiInclude += QFileInfo(uiFile).completeBaseName();
    uiInclude += QLatin1String(".h");

    // 1) Header file
    QTextStream headerStr(header);
    headerStr << license << "#ifndef " << guard
              << "\n#define " <<  guard << '\n' << '\n';

    // Include 'ui_'
    if (embedding != FormClassWizardGenerationParameters::PointerAggregatedUiClass) {
        Utils::writeIncludeFileDirective(uiInclude, false, headerStr);
    } else {
        // Todo: Can we obtain the header from the code model for custom widgets?
        // Alternatively, from Designer.
        if (formBaseClass.startsWith(QLatin1Char('Q'))) {
            QString baseInclude = formBaseClass;
            if (generationParameters.includeQtModule())
                baseInclude.insert(0, QLatin1String("QtGui/"));
            Utils::writeIncludeFileDirective(baseInclude, true, headerStr);
        }
    }

    const QString namespaceIndent = Utils::writeOpeningNameSpaces(namespaceList,
                                                                        generationParameters.indentNamespace() ? indent : QString(),
                                                                        headerStr);

    // Forward-declare the UI class
    if (embedding == FormClassWizardGenerationParameters::PointerAggregatedUiClass) {
          headerStr << '\n'
                  << namespaceIndent << "namespace " <<  uiNamespaceC << " {\n"
                  << namespaceIndent << indent << "class " << Internal::FormTemplateWizardPage::stripNamespaces(uiClassName) << ";\n"
                  << namespaceIndent << "}\n";
    }

    // Class declaration
    headerStr << '\n' << namespaceIndent << "class " << unqualifiedClassName
              << " : public " << formBaseClass;
    if (embedding == FormClassWizardGenerationParameters::InheritedUiClass) {
        headerStr << ", private " << uiClassName;
    }
    headerStr << " {\n" << namespaceIndent << indent << "Q_OBJECT\n"
              << namespaceIndent << "public:\n"
              << namespaceIndent << indent << "explicit " << unqualifiedClassName << "(QWidget *parent = 0);\n";
    if (embedding == FormClassWizardGenerationParameters::PointerAggregatedUiClass)
        headerStr << namespaceIndent << indent << "~" << unqualifiedClassName << "();\n";
    // retranslation
    if (generationParameters.retranslationSupport())
        headerStr << '\n' << namespaceIndent << "protected:\n"
                  << namespaceIndent << indent << "void changeEvent(QEvent *e);\n";
    // Member variable
    if (embedding != FormClassWizardGenerationParameters::InheritedUiClass) {
        headerStr << '\n' << namespaceIndent << "private:\n"
                  << namespaceIndent << indent << uiClassName << ' ';
        if (embedding == FormClassWizardGenerationParameters::PointerAggregatedUiClass)
            headerStr << '*';
        headerStr << uiMemberC << ";\n";
    }
    headerStr << namespaceIndent << "};\n\n";
    Utils::writeClosingNameSpaces(namespaceList, generationParameters.indentNamespace() ? indent : QString(), headerStr);
    headerStr << "#endif // "<<  guard << '\n';

    // 2) Source file
    QTextStream sourceStr(source);
    sourceStr << license;
    Utils::writeIncludeFileDirective(headerFile, false, sourceStr);
    if (embedding == FormClassWizardGenerationParameters::PointerAggregatedUiClass)
        Utils::writeIncludeFileDirective(uiInclude, false, sourceStr);
    // NameSpaces(
    Utils::writeOpeningNameSpaces(namespaceList, generationParameters.indentNamespace() ? indent : QString(), sourceStr);
    // Constructor with setupUi
    sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName << "(QWidget *parent) :\n"
               << namespaceIndent << indent << formBaseClass << "(parent)";
    if (embedding == FormClassWizardGenerationParameters::PointerAggregatedUiClass)
        sourceStr << ",\n"  << namespaceIndent << indent <<  uiMemberC << "(new " << uiClassName << ")\n";
    sourceStr <<  namespaceIndent << "{\n" <<  namespaceIndent << indent;
    writeUiMemberAccess(generationParameters, sourceStr);
    sourceStr <<  "setupUi(this);\n" << namespaceIndent << "}\n";
    // Deleting destructor for ptr
    if (embedding == FormClassWizardGenerationParameters::PointerAggregatedUiClass) {
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
    Utils::writeClosingNameSpaces(namespaceList, generationParameters.indentNamespace() ? indent : QString(), sourceStr);
    return true;
}

bool FormClassWizardParameters::generateCpp(const FormClassWizardGenerationParameters &fgp,
                                            QString *header, QString *source, int indentation) const
{
    return m_d->generateCpp(fgp, header, source, indentation);
}


} // namespace Designer
