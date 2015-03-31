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

#include "customwizardpage.h"
#include "customwizardparameters.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/textfieldcheckbox.h>
#include <utils/textfieldcombobox.h>

#include <QRegExp>
#include <QDebug>
#include <QDir>
#include <QDate>
#include <QTime>

#include <QWizardPage>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QRegExpValidator>
#include <QComboBox>
#include <QTextEdit>
#include <QSpacerItem>

enum { debug = 0 };

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

/*!
    \class ProjectExplorer::Internal::CustomWizardFieldPage
    \brief The CustomWizardFieldPage class is a simple custom wizard page
    presenting the fields to be used
    as page 2 of a BaseProjectWizardDialog if there are any fields.

    Uses the 'field' functionality of QWizard.
    Implements validatePage() as the field logic cannot be tied up
    with additional validation. Performs checking of the Javascript-based
    validation rules of the parameters and displays error messages in a red
    warning label.

    \sa ProjectExplorer::CustomWizard
*/

CustomWizardFieldPage::LineEditData::LineEditData(QLineEdit* le, const QString &defText, const QString &pText) :
    lineEdit(le), defaultText(defText), placeholderText(pText)
{
}

CustomWizardFieldPage::TextEditData::TextEditData(QTextEdit* le, const QString &defText) :
    textEdit(le), defaultText(defText)
{
}

CustomWizardFieldPage::PathChooserData::PathChooserData(PathChooser* pe, const QString &defText) :
    pathChooser(pe), defaultText(defText)
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
    foreach (const CustomWizardField &f, parameters->fields)
        addField(f);
    vLayout->addLayout(m_formLayout);
    m_errorLabel->setVisible(false);
    m_errorLabel->setStyleSheet(QLatin1String("background: red"));
    vLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    vLayout->addWidget(m_errorLabel);
    setLayout(vLayout);
    if (!parameters->fieldPageTitle.isEmpty())
        setTitle(parameters->fieldPageTitle);
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
    m_errorLabel->clear();
    m_errorLabel->setVisible(false);
}

/*!
    Creates a widget based on the control attributes map and registers it with
    the QWizard.
*/

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
    if (spansRow)
        m_formLayout->addRow(fieldWidget);
    else
        addRow(field.description, fieldWidget);
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
    // Connect to completeChanged() for derived classes that reimplement isComplete()
    connect(combo, SIGNAL(text4Changed(QString)), SIGNAL(completeChanged()));
    return combo;
} // QComboBox

QWidget *CustomWizardFieldPage::registerTextEdit(const QString &fieldName,
                                                 const CustomWizardField &field)
{
    QTextEdit *textEdit = new QTextEdit;
    // Suppress formatting by default (inverting QTextEdit's default value) when
    // pasting from Bug tracker, etc.
    const bool acceptRichText = field.controlAttributes.value(QLatin1String("acceptRichText")) == QLatin1String("true");
    textEdit->setAcceptRichText(acceptRichText);
    // Connect to completeChanged() for derived classes that reimplement isComplete()
    registerField(fieldName, textEdit, "plainText", SIGNAL(textChanged()));
    connect(textEdit, SIGNAL(textChanged()), SIGNAL(completeChanged()));
    const QString defaultText = field.controlAttributes.value(QLatin1String("defaulttext"));
    m_textEdits.push_back(TextEditData(textEdit, defaultText));
    return textEdit;
} // QTextEdit

QWidget *CustomWizardFieldPage::registerPathChooser(const QString &fieldName,
                                                 const CustomWizardField &field)
{
    PathChooser *pathChooser = new PathChooser;
    const QString expectedKind = field.controlAttributes.value(QLatin1String("expectedkind")).toLower();
    if (expectedKind == QLatin1String("existingdirectory"))
        pathChooser->setExpectedKind(PathChooser::ExistingDirectory);
    else if (expectedKind == QLatin1String("directory"))
        pathChooser->setExpectedKind(PathChooser::Directory);
    else if (expectedKind == QLatin1String("file"))
        pathChooser->setExpectedKind(PathChooser::File);
    else if (expectedKind == QLatin1String("existingcommand"))
        pathChooser->setExpectedKind(PathChooser::ExistingCommand);
    else if (expectedKind == QLatin1String("command"))
        pathChooser->setExpectedKind(PathChooser::Command);
    else if (expectedKind == QLatin1String("any"))
        pathChooser->setExpectedKind(PathChooser::Any);
    pathChooser->setHistoryCompleter(QString::fromLatin1("PE.Custom.") + m_parameters->id + QLatin1Char('.') + field.name);

    registerField(fieldName, pathChooser, "path", SIGNAL(changed(QString)));
    // Connect to completeChanged() for derived classes that reimplement isComplete()
    connect(pathChooser, SIGNAL(changed(QString)), SIGNAL(completeChanged()));
    const QString defaultText = field.controlAttributes.value(QLatin1String("defaulttext"));
    m_pathChoosers.push_back(PathChooserData(pathChooser, defaultText));
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
    // Connect to completeChanged() for derived classes that reimplement isComplete()
    connect(checkBox, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    return checkBox;
}

QWidget *CustomWizardFieldPage::registerLineEdit(const QString &fieldName,
                                                 const CustomWizardField &field)
{
    QLineEdit *lineEdit = new QLineEdit;

    const QString validationRegExp = field.controlAttributes.value(QLatin1String("validator"));
    if (!validationRegExp.isEmpty()) {
        QRegExp re(validationRegExp);
        if (re.isValid())
            lineEdit->setValidator(new QRegExpValidator(re, lineEdit));
        else
            qWarning("Invalid custom wizard field validator regular expression %s.", qPrintable(validationRegExp));
    }
    registerField(fieldName, lineEdit, "text", SIGNAL(textEdited(QString)));
    // Connect to completeChanged() for derived classes that reimplement isComplete()
    connect(lineEdit, SIGNAL(textEdited(QString)), SIGNAL(completeChanged()));

    const QString defaultText = field.controlAttributes.value(QLatin1String("defaulttext"));
    const QString placeholderText = field.controlAttributes.value(QLatin1String("placeholdertext"));
    m_lineEdits.push_back(LineEditData(lineEdit, defaultText, placeholderText));
    return lineEdit;
}

void CustomWizardFieldPage::initializePage()
{
    QWizardPage::initializePage();
    clearError();
    foreach (const LineEditData &led, m_lineEdits) {
        if (!led.userChange.isNull()) {
            led.lineEdit->setText(led.userChange);
        } else if (!led.defaultText.isEmpty()) {
            QString defaultText = led.defaultText;
            CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
            led.lineEdit->setText(defaultText);
        }
        if (!led.placeholderText.isEmpty())
            led.lineEdit->setPlaceholderText(led.placeholderText);
    }
    foreach (const TextEditData &ted, m_textEdits) {
        if (!ted.userChange.isNull()) {
            ted.textEdit->setText(ted.userChange);
        } else if (!ted.defaultText.isEmpty()) {
            QString defaultText = ted.defaultText;
            CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
            ted.textEdit->setText(defaultText);
        }
    }
    foreach (const PathChooserData &ped, m_pathChoosers) {
        if (!ped.userChange.isNull()) {
            ped.pathChooser->setPath(ped.userChange);
        } else if (!ped.defaultText.isEmpty()) {
            QString defaultText = ped.defaultText;
            CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
            ped.pathChooser->setPath(defaultText);
        }
    }
}

void CustomWizardFieldPage::cleanupPage()
{
    for (int i = 0; i < m_lineEdits.count(); ++i) {
        LineEditData &led = m_lineEdits[i];
        QString defaultText = led.defaultText;
        CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
        if (led.lineEdit->text() != defaultText)
            led.userChange = led.lineEdit->text();
        else
            led.userChange.clear();

    }
    for (int i= 0; i < m_textEdits.count(); ++i) {
        TextEditData &ted = m_textEdits[i];
        QString defaultText = ted.defaultText;
        CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
        if (ted.textEdit->toHtml() != ted.defaultText && ted.textEdit->toPlainText() != ted.defaultText)
            ted.userChange = ted.textEdit->toHtml();
        else
            ted.userChange.clear();
    }
    for (int i= 0; i < m_pathChoosers.count(); ++i) {
        PathChooserData &ped = m_pathChoosers[i];
        QString defaultText = ped.defaultText;
        CustomWizardContext::replaceFields(m_context->baseReplacements, &defaultText);
        if (ped.pathChooser->path() != ped.defaultText)
            ped.userChange = ped.pathChooser->path();
        else
            ped.userChange.clear();
    }
    QWizardPage::cleanupPage();
}

bool CustomWizardFieldPage::validatePage()
{
    clearError();
    // Check line edits with validators
    foreach (const LineEditData &led, m_lineEdits) {
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
    foreach (const Internal::CustomWizardField &field, f) {
        const QString value = w->field(field.name).toString();
        fieldReplacementMap.insert(field.name, value);
    }
    // Insert paths for generator scripts.
    fieldReplacementMap.insert(QLatin1String("Path"), QDir::toNativeSeparators(ctx->path));
    fieldReplacementMap.insert(QLatin1String("TargetPath"), QDir::toNativeSeparators(ctx->targetPath));

    return fieldReplacementMap;
}

/*!
    \class ProjectExplorer::Internal::CustomWizardPage
    \brief The CustomWizardPage class provides a custom wizard page presenting
    the fields to be used and a path chooser
    at the bottom (for use by "class"/"file" wizards).

    Does validation on the Path chooser only (as the other fields can by validated by regexps).

    \sa ProjectExplorer::CustomWizard
*/

CustomWizardPage::CustomWizardPage(const QSharedPointer<CustomWizardContext> &ctx,
                                   const QSharedPointer<CustomWizardParameters> &parameters,
                                   QWidget *parent) :
    CustomWizardFieldPage(ctx, parameters, parent),
    m_pathChooser(new PathChooser)
{
    m_pathChooser->setHistoryCompleter(QLatin1String("PE.ProjectDir.History"));
    addRow(tr("Path:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(validChanged(bool)), this, SIGNAL(completeChanged()));
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
    return m_pathChooser->isValid() && CustomWizardFieldPage::isComplete();
}

} // namespace Internal
} // namespace ProjectExplorer
