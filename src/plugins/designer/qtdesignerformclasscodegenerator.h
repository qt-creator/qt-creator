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

#ifndef QTDESIGNERFORMCLASSCODEGENERATOR_H
#define QTDESIGNERFORMCLASSCODEGENERATOR_H

#include <QString>
#include <QVariant>
#include <QObject>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Designer {
class FormClassWizardParameters;
namespace Internal {

// How to embed the Ui::Form class.
enum UiClassEmbedding
{
    PointerAggregatedUiClass, // "Ui::Form *m_ui";
    AggregatedUiClass,        // "Ui::Form m_ui";
    InheritedUiClass          // "...private Ui::Form..."
};

// Parameters influencing the code generation to be used in settings page.
struct FormClassWizardGenerationParameters
{
    FormClassWizardGenerationParameters();
    bool equals(const FormClassWizardGenerationParameters &rhs) const;

    void fromSettings(const QSettings *);
    void toSettings(QSettings *) const;

    UiClassEmbedding embedding;
    bool retranslationSupport; // Add handling for language change events
    bool includeQtModule; // Include "<QtGui/[Class]>" or just "<[Class]>"
    bool addQtVersionCheck; // Include #ifdef when using "#include <QtGui/..."
    bool indentNamespace;
};

inline bool operator==(const FormClassWizardGenerationParameters &p1, const FormClassWizardGenerationParameters &p2) { return p1.equals(p2); }
inline bool operator!=(const FormClassWizardGenerationParameters &p1, const FormClassWizardGenerationParameters &p2) { return !p1.equals(p2); }

} // namespace Internal

// Publicly registered service to generate the code for a form class
// (See PluginManager::Invoke) to be accessed by Qt4ProjectManager.
class QtDesignerFormClassCodeGenerator : public QObject
{
    Q_OBJECT
public:
    explicit QtDesignerFormClassCodeGenerator(QObject *parent = 0);

    static bool generateCpp(const FormClassWizardParameters &parameters,
                            QString *header, QString *source, int indentation = 4);

    // Generate form class code according to settings.
    Q_INVOKABLE QVariant generateFormClassCode(const Designer::FormClassWizardParameters &parameters);
};
} // namespace Designer

#endif // QTDESIGNERFORMCLASSCODEGENERATOR_H
