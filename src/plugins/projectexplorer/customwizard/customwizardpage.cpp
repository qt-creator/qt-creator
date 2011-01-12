/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "customwizardpage.h"
#include "customwizardparameters.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QtCore/QRegExp>
#include <QtCore/QDebug>
#include <QtCore/QDir>

#include <QtGui/QWizardPage>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QRegExpValidator>
#include <QtGui/QComboBox>
#include <QtGui/QTextEdit>
#include <QtGui/QSpacerItem>

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// ----------- TextFieldComboBox
TextFieldComboBox::TextFieldComboBox(QWidget *parent) :
    QComboBox(parent)
{
    setEditable(false);
    connect(this, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotCurrentIndexChanged(int)));
}

QString TextFieldComboBox::text() const
{
    return valueAt(currentIndex());
}

void TextFieldComboBox::setText(const QString &s)
{
    const int index = findData(QVariant(s), Qt::UserRole);
    if (index != -1 && index != currentIndex())
        setCurrentIndex(index);
}

void TextFieldComboBox::slotCurrentIndexChanged(int i)
{
    emit text4Changed(valueAt(i));
}

void TextFieldComboBox::setItems(const QStringList &displayTexts,
                                 const QStringList &values)
{
    QTC_ASSERT(displayTexts.size() == values.size(), return)
    clear();
    addItems(displayTexts);
    const int count = values.count();
    for (int i = 0; i < count; i++)
        setItemData(i, QVariant(values.at(i)), Qt::UserRole);
}

QString TextFieldComboBox::valueAt(int i) const
{
    return i >= 0 && i < count() ? itemData(i, Qt::UserRole).toString() : QString();
}

// -------------- TextCheckBox
TextFieldCheckBox::TextFieldCheckBox(const QString &text, QWidget *parent) :
        QCheckBox(text, parent),
        m_trueText(QLatin1String("true")), m_falseText(QLatin1String("false"))
{
    connect(this, SIGNAL(stateChanged(int)), this, SLOT(slotStateChanged(int)));
}

QString TextFieldCheckBox::text() const
{
    return isChecked() ? m_trueText : m_falseText;
}

void TextFieldCheckBox::setText(const QString &s)
{
    setChecked(s == m_trueText);
}

void TextFieldCheckBox::slotStateChanged(int cs)
{
    emit textChanged(cs == Qt::Checked ? m_trueText : m_falseText);
}

// --------------- CustomWizardFieldPage

CustomWizardFieldPage::LineEditData::LineEditData(QLineEdit* le, const QString &defText) :
    lineEdit(le), defaultText(defText)
{
}

CustomWizardFieldPage::TextEditData::TextEditData(QTextEdit* le, const QString &defText) :
    textEdit(le), defaultText(defText)
{
}

CustomWizardFieldPage::CustomWizardFieldPage(const QSharedPointer<CustomWizardContext> &ctx,
                                             const QSharedPointer<CustomWizardParameters> &parameters,
                                             QWidget *parent) :
    QWizardPage(parent),
    m_parameters(parameters),
    m_context(ctx),
    m_formLayout(new QFormLayout),
    m_errorLabel(new QLabel)
{
    QVBoxLayout *vLayout = new QVBoxLayout;
    m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    if (debug)
        qDebug() << Q_FUNC_INFO << parameters->fields.size();
    foreach(const CustomWizardField &f, parameters->fields)
        addField(f);
    vLayout->addLayout(m_formLayout);
    m_errorLabel->setVisible(false);
    m_errorLabel->setStyleSheet(QLatin1String("background: red"));
    vLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    vLayout->addWidget(m_errorLabel);
    setLayout(vLayout);
}

CustomWizardFieldPage::~CustomWizardFieldPage()
{
}

void CustomWizardFieldPage::addRow(const QString &name, QWidget *w)
{
    m_formLayout->addRow(name, w);
}

void CustomWizardFieldPage::showError(const QString &m)
{
    m_errorLabel->setText(m);
    m_errorLabel->setVisible(true);
}

void CustomWizardFieldPage::clearError()
{
    m_errorLabel->setText(QString());
    m_errorLabel->setVisible(false);
}

// Create widget a control based  on the control attributes map
// and register it with the QWizard.
void CustomWizardFieldPage::addField(const CustomWizardField &field)\
{
    //  Register field, indicate mandatory by '*' (only when registering)
    QString fieldName = field.name;
    if (field.mandatory)
        fieldName += QLatin1Char('*');
    bool spansRow = false;
    // Check known classes: QComboBox
    const QString className = field.controlAttributes.value(QLatin1String("class"));
    QWidget *fieldWidget = 0;
    if (className == QLatin1String("QComboBox")) {
        fieldWidget = registerComboBox(fieldName, field);
    } else if (className == QLatin1String("QTextEdit")) {
        fieldWidget = registerTextEdit(fieldName, field);
    } else if (className == QLatin1String("Utils::PathChooser")) {
        fieldWidget = registerPathChooser(fieldName, field);
    } else if (className == QLatin1String("QCheckBox")) {
        fieldWidget = registerCheckBox(fieldName, field.description, field);
        spansRow = true; // Do not create a label for the checkbox.
    } else {
        fieldWidget = registerLineEdit(fieldName, field);
    }
    if (spansRow) {
        m_formLayout->addRow(fieldWidget);
    } else {
        addRow(field.description, fieldWidget);
    }
}

// Return the list of values and display texts for combo
static void comboChoices(const CustomWizardField::ControlAttributeMap &controlAttributes,
                  QStringList *values, QStringList *displayTexts)
{
    typedef CustomWizardField::ControlAttributeMap::ConstIterator AttribMapConstIt;

    values->clear();
    displayTexts->clear();
    // Pre 2.2 Legacy: "combochoices" attribute with a comma-separated list, for
    // display == value.
    const AttribMapConstIt attribConstEnd = controlAttributes.constEnd();
    const AttribMapConstIt choicesIt = controlAttributes.constFind(QLatin1String("combochoices"));
    if (choicesIt != attribConstEnd) {
        const QString &choices = choicesIt.value();
        if (!choices.isEmpty())
            *values = *displayTexts = choices.split(QLatin1Char(','));
        return;
    }
    // From 2.2 on: Separate lists of value and text. Add all values found.
    for (int i = 0; ; i++) {
        const QString valueKey = CustomWizardField::comboEntryValueKey(i);
        const AttribMapConstIt valueIt = controlAttributes.constFind(valueKey);
        if (valueIt == attribConstEnd)
            break;
        values->push_back(valueIt.value());
        const QString textKey = CustomWizardField::comboEntryTextKey(i);
        displayTexts->push_back(controlAttributes.value(textKey));
    }
}

QWidget *CustomWizardFieldPage::registerComboBox(const QString &fieldName,
                                                 const CustomWizardField &field)
{
    TextFieldComboBox *combo = new TextFieldComboBox;
    do { // Set up items and current index
        QStringList values;
        QStringList displayTexts;
        comboChoices(field.controlAttributes, &values, &displayTexts);
        combo->setItems(displayTexts, values);
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

QWidget *CustomWizardFieldPage::registerTextEdit(const QString &fieldName,
                                                 const CustomWizardField &field)
{
    QTextEdit *textEdit = new QTextEdit;
    registerField(fieldName, textEdit, "plainText", SIGNAL(textChanged(QString)));
    const QString defaultText = field.controlAttributes.value(QLatin1String("defaulttext"));
    m_textEdits.push_back(TextEditData(textEdit, defaultText));
    return textEdit;
} // QTextEdit

QWidget *CustomWizardFieldPage::registerPathChooser(const QString &fieldName,
                                                 const CustomWizardField & /*field*/)
{
    Utils::PathChooser *pathChooser = new Utils::PathChooser;
    registerField(fieldName, pathChooser, "path", SIGNAL(changed(QString)));
    return pathChooser;
} // Utils::PathChooser

QWidget *CustomWizardFieldPage::registerCheckBox(const QString &fieldName,
                                                 const QString &fieldDescription,
                                                 const CustomWizardField &field)
{
    typedef CustomWizardField::ControlAttributeMap::const_iterator AttributeMapConstIt;

    TextFieldCheckBox *checkBox = new TextFieldCheckBox(fieldDescription);
    const bool defaultValue = field.controlAttributes.value(QLatin1String("defaultvalue")) == QLatin1String("true");
    checkBox->setChecked(defaultValue);
    const AttributeMapConstIt trueTextIt = field.controlAttributes.constFind(QLatin1String("truevalue"));
    if (trueTextIt != field.controlAttributes.constEnd()) // Also set empty texts
        checkBox->setTrueText(trueTextIt.value());
    const AttributeMapConstIt falseTextIt = field.controlAttributes.constFind(QLatin1String("falsevalue"));
    if (falseTextIt != field.controlAttributes.constEnd()) // Also set empty texts
        checkBox->setFalseText(falseTextIt.value());
    registerField(fieldName, checkBox, "text", SIGNAL(textChanged(QString)));
    return checkBox;
}

QWidget *CustomWizardFieldPage::registerLineEdit(const QString &fieldName,
                                                 const CustomWizardField &field)
{
    QLineEdit *lineEdit = new QLineEdit;

    const QString validationRegExp = field.controlAttributes.value(QLatin1String("validator"));
    if (!validationRegExp.isEmpty()) {
        QRegExp re(validationRegExp);
        if (re.isValid()) {
            lineEdit->setValidator(new QRegExpValidator(re, lineEdit));
        } else {
            qWarning("Invalid custom wizard field validator regular expression %s.", qPrintable(validationRegExp));
        }
    }
    registerField(fieldName, lineEdit, "text", SIGNAL(textEdited(QString)));

    const QString defaultText = field.controlAttributes.value(QLatin1String("defaulttext"));
    m_lineEdits.push_back(LineEditData(lineEdit, defaultText));
    return lineEdit;
}

void CustomWizardFieldPage::initializePage()
{
    QWizardPage::initializePage();
    clearError();
    // Note that the field mechanism will always restore the value
    // set on it when entering the page, so, there is no point in
    // trying to preserve user modifications of the text.
    foreach(const LineEditData &led, m_lineEdits) {
        if (!led.defaultText.isEmpty()) {
            QString defaultText = led.defaultText;
            CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
            led.lineEdit->setText(defaultText);
        }
    }
    foreach(const TextEditData &ted, m_textEdits) {
        if (!ted.defaultText.isEmpty()) {
            QString defaultText = ted.defaultText;
            CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
            ted.textEdit->setText(defaultText);
        }
    }
}

bool CustomWizardFieldPage::validatePage()
{
    clearError();
    // Check line edits with validators
    foreach(const LineEditData &led, m_lineEdits) {
        if (const QValidator *val = led.lineEdit->validator()) {
            int pos = 0;
            QString text = led.lineEdit->text();
            if (val->validate(text, pos) != QValidator::Acceptable) {
                led.lineEdit->setFocus();
                return false;
            }
        }
    }
    // Any user validation rules -> Check all and display messages with
    // place holders applied.
    if (!m_parameters->rules.isEmpty()) {
        const QMap<QString, QString> values = replacementMap(wizard(), m_context, m_parameters->fields);
        QString message;
        if (!CustomWizardValidationRule::validateRules(m_parameters->rules, values, &message)) {
            showError(message);
            return false;
        }
    }
    return QWizardPage::validatePage();
}

QMap<QString, QString> CustomWizardFieldPage::replacementMap(const QWizard *w,
                                                             const QSharedPointer<CustomWizardContext> &ctx,
                                                             const FieldList &f)
{
    QMap<QString, QString> fieldReplacementMap = ctx->baseReplacements;
    foreach(const Internal::CustomWizardField &field, f) {
        const QString value = w->field(field.name).toString();
        fieldReplacementMap.insert(field.name, value);
    }
    // Insert paths for generator scripts.
    fieldReplacementMap.insert(QLatin1String("Path"), QDir::toNativeSeparators(ctx->path));
    fieldReplacementMap.insert(QLatin1String("TargetPath"), QDir::toNativeSeparators(ctx->targetPath));
    return fieldReplacementMap;
}

// --------------- CustomWizardPage

CustomWizardPage::CustomWizardPage(const QSharedPointer<CustomWizardContext> &ctx,
                                   const QSharedPointer<CustomWizardParameters> &parameters,
                                   QWidget *parent) :
    CustomWizardFieldPage(ctx, parameters, parent),
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
