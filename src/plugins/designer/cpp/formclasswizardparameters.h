/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FORMCLASSWIZARDPARAMETERS_H
#define FORMCLASSWIZARDPARAMETERS_H

#include "../designer_export.h"
#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Designer {

class FormClassWizardGenerationParametersPrivate;
class FormClassWizardParametersPrivate;

// Parameters influencing the code generation.
class DESIGNER_EXPORT FormClassWizardGenerationParameters {
public:
    // How to embed the Ui::Form class.
    enum UiClassEmbedding {
        PointerAggregatedUiClass, // "Ui::Form *m_ui";
        AggregatedUiClass,        // "Ui::Form m_ui";
        InheritedUiClass          // "...private Ui::Form..."
    };

    FormClassWizardGenerationParameters();
    ~FormClassWizardGenerationParameters();
    FormClassWizardGenerationParameters(const FormClassWizardGenerationParameters&);
    FormClassWizardGenerationParameters &operator=(const FormClassWizardGenerationParameters &);

    void fromSettings(const QSettings *);
    void toSettings(QSettings *) const;

    UiClassEmbedding embedding() const;
    void setEmbedding(UiClassEmbedding e);

    bool retranslationSupport() const; // Add handling for language change events
    void setRetranslationSupport(bool v);

    bool includeQtModule() const;      // Include "<QtGui/[Class]>" or just "<[Class]>"
    void setIncludeQtModule(bool v);

    bool indentNamespace() const;
    void setIndentNamespace(bool v);

    bool equals(const FormClassWizardGenerationParameters &rhs) const;

private:
    QSharedDataPointer<FormClassWizardGenerationParametersPrivate> m_d;
};

inline bool operator==(const FormClassWizardGenerationParameters &p1, const FormClassWizardGenerationParameters &p2) { return p1.equals(p2); }
inline bool operator!=(const FormClassWizardGenerationParameters &p1, const FormClassWizardGenerationParameters &p2) { return !p1.equals(p2); }

// Parameters required to generate the code part of a form class with
// helpers for XML-processing ui templates.
class DESIGNER_EXPORT FormClassWizardParameters {
public:
    FormClassWizardParameters();
    ~FormClassWizardParameters();
    FormClassWizardParameters(const FormClassWizardParameters &);
    FormClassWizardParameters &operator=(const FormClassWizardParameters &);

    bool generateCpp(const FormClassWizardGenerationParameters &fgp,
                     QString *header, QString *source, int indentation = 4) const;

    // Helper to parse UI XML forms to determine:
    // 1) The ui class name from "<class>Designer::Internal::FormClassWizardPage</class>"
    // 2) the base class from: widget class="QWizardPage"...
    static bool getUIXmlData(const QString &uiXml, QString *formBaseClass, QString *uiClassName);
    // Helper to change the class name in a UI XML form
    static QString changeUiClassName(const QString &uiXml, const QString &newUiClassName);

    QString uiTemplate() const;
    void setUiTemplate(const QString &);

    QString className() const;
    void setClassName(const QString &);

    QString path() const;
    void setPath(const QString &);

    QString sourceFile() const;
    void setSourceFile(const QString &);

    QString headerFile() const;
    void setHeaderFile(const QString &);

    QString uiFile() const;
    void setUiFile(const QString &);

private:
    QSharedDataPointer<FormClassWizardParametersPrivate> m_d;
};

} // namespace Designer

#endif // FORMCLASSWIZARDPARAMETERS_H
