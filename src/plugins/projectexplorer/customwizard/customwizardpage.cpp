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

#include "customwizardpage.h"
#include "customwizardparameters.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QtCore/QRegExp>
#include <QtCore/QDebug>

#include <QtGui/QWizardPage>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QRegExpValidator>
#include <QtGui/QComboBox>

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

TextFieldComboBox::TextFieldComboBox(QWidget *parent) :
    QComboBox(parent)
{
    setEditable(false);
    connect(this, SIGNAL(currentIndexChanged(QString)), this, SIGNAL(textChanged(QString)));
}

void TextFieldComboBox::setText(const QString &s)
{
    const int index = findText(s);
    if (index != -1 && index != currentIndex())
        setCurrentIndex(index);
}

// --------------- CustomWizardFieldPage
CustomWizardFieldPage::CustomWizardFieldPage(const FieldList &fields,
                                             QWidget *parent) :
    QWizardPage(parent),
    m_formLayout(new QFormLayout)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << fields.size();
    foreach(const CustomWizardField &f, fields)
        addField(f);
    setLayout(m_formLayout);
}

void CustomWizardFieldPage::addRow(const QString &name, QWidget *w)
{
    m_formLayout->addRow(name, w);
}

// Create widget a control based  on the control attributes map
// and register it with the QWizard.
QWidget *CustomWizardFieldPage::registerControl(const CustomWizardField &field)
{
    //  Register field,  Indicate mandatory by '*' (only when registering)
    QString fieldName = field.name;
    if (field.mandatory)
        fieldName += QLatin1Char('*');
    // Check known classes: QComboBox
    const QString className = field.controlAttributes.value(QLatin1String("class"));
    if (className == QLatin1String("QComboBox")) {
        TextFieldComboBox *combo = new TextFieldComboBox;
        // Set up items
        do {
            const QString choices = field.controlAttributes.value(QLatin1String("combochoices"));
            if (choices.isEmpty())
                break;
            combo->addItems(choices.split(QLatin1Char(',')));
            bool ok;
            const QString currentIndexS = field.controlAttributes.value(QLatin1String("defaultindex"));
            if (currentIndexS.isEmpty())
                break;
            const int currentIndex = currentIndexS.toInt(&ok);
            if (!ok || currentIndex < 0 || currentIndex >= combo->count())
                break;
            combo->setCurrentIndex(currentIndex);
        } while (false);
        registerField(fieldName, combo, "text", SIGNAL(text4Changed(QString)));
        return combo;
    } // QComboBox
    // Default to QLineEdit
    QLineEdit *lineEdit = new QLineEdit;
    const QString validationRegExp = field.controlAttributes.value(QLatin1String("validator"));
    if (!validationRegExp.isEmpty()) {
        QRegExp re(validationRegExp);
        if (re.isValid()) {
            lineEdit->setValidator(new QRegExpValidator(re, lineEdit));
        } else {
            qWarning("Invalid custom wizard field validator regular expression %s.", qPrintable(validationRegExp));
        }
        m_validatorLineEdits.push_back(lineEdit);
    }
    lineEdit->setText(field.controlAttributes.value(QLatin1String("defaulttext")));
    registerField(fieldName, lineEdit, "text", SIGNAL(textEdited(QString)));
    return lineEdit;
}

void CustomWizardFieldPage::addField(const CustomWizardField &field)
{
    addRow(field.description, registerControl(field));
}

bool CustomWizardFieldPage::validatePage()
{
    // Check line edits with validators
    foreach(QLineEdit *le, m_validatorLineEdits) {
        int pos = 0;
        const QValidator *val = le->validator();
        QTC_ASSERT(val, return false);
        QString text = le->text();
        if (val->validate(text, pos) != QValidator::Acceptable) {
            le->setFocus();
            return false;
        }
    }
    return QWizardPage::validatePage();
}

// --------------- CustomWizardPage

CustomWizardPage::CustomWizardPage(const FieldList &f,
                                   QWidget *parent) :
    CustomWizardFieldPage(f, parent),
    m_pathChooser(new Utils::PathChooser)
{
    addRow(tr("Path:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(validChanged()), this, SIGNAL(completeChanged()));
}

QString CustomWizardPage::path() const
{
    return m_pathChooser->path();
}

void CustomWizardPage::setPath(const QString &path)
{
    m_pathChooser->setPath(path);
}

bool CustomWizardPage::isComplete() const
{
    return m_pathChooser->isValid();
}

} // namespace Internal
} // namespace ProjectExplorer
