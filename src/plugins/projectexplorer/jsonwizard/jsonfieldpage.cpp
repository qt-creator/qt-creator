/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "jsonfieldpage.h"

#include "jsonwizard.h"
#include "jsonwizardexpander.h"
#include "jsonwizardfactory.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/textfieldcheckbox.h>
#include <utils/textfieldcombobox.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVariant>
#include <QVariantMap>
#include <QVBoxLayout>

static const char NAME_KEY[] = "name";
static const char DISPLAY_NAME_KEY[] = "trDisplayName";
static const char MANDATORY_KEY[] = "mandatory";
static const char VISIBLE_KEY[] = "visible";
static const char ENABLED_KEY[] = "enabled";
static const char SPAN_KEY[] = "span";
static const char TYPE_KEY[] = "type";
static const char DATA_KEY[] = "data";


namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

static JsonFieldPage::Field *createFieldData(const QString &type)
{
    if (type == QLatin1String("Label"))
        return new JsonFieldPage::LabelField;
    else if (type == QLatin1String("Spacer"))
        return new JsonFieldPage::SpacerField;
    else if (type == QLatin1String("LineEdit"))
        return new JsonFieldPage::LineEditField;
    else if (type == QLatin1String("TextEdit"))
        return new JsonFieldPage::TextEditField;
    else if (type == QLatin1String("PathChooser"))
        return new JsonFieldPage::PathChooserField;
    else if (type == QLatin1String("CheckBox"))
        return new JsonFieldPage::CheckBoxField;
    else if (type == QLatin1String("ComboBox"))
        return new JsonFieldPage::ComboBoxField;
    return 0;
}

// --------------------------------------------------------------------
// JsonFieldPage::FieldData:
// --------------------------------------------------------------------

JsonFieldPage::Field *JsonFieldPage::Field::parse(const QVariant &input, QString *errorMessage)
{
    if (input.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field is not an object.");
        return 0;
    }

    QVariantMap tmp = input.toMap();
    const QString name = tmp.value(QLatin1String(NAME_KEY)).toString();
    if (name.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field has no name.");
        return 0;
    }
    const QString type = tmp.value(QLatin1String(TYPE_KEY)).toString();
    if (type.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field '%1' has no type.").arg(name);
        return 0;
    }

    Field *data = createFieldData(type);
    if (!data) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field '%1' has unsupported type '%2'.")
                .arg(name).arg(type);
        return 0;
    }
    data->name = name;

    data->m_visibleExpression = tmp.value(QLatin1String(VISIBLE_KEY), true);
    data->m_enabledExpression = tmp.value(QLatin1String(ENABLED_KEY), true);
    data->mandatory = tmp.value(QLatin1String(MANDATORY_KEY), true).toBool();
    data->span = tmp.value(QLatin1String(SPAN_KEY), false).toBool();

    data->displayName = JsonWizardFactory::localizedString(tmp.value(QLatin1String(DISPLAY_NAME_KEY)).toString());

    QVariant dataVal = tmp.value(QLatin1String(DATA_KEY));
    if (!data->parseData(dataVal, errorMessage)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "When parsing Field '%1': %2")
                .arg(name).arg(*errorMessage);
        delete data;
        return 0;
    }

    return data;
}

void JsonFieldPage::Field::createWidget(JsonFieldPage *page)
{
    QWidget *w = widget(displayName);
    w->setObjectName(name);
    QFormLayout *layout = page->layout();

    if (suppressName())
        layout->addWidget(w);
    else if (span)
        layout->addRow(w);
    else
        layout->addRow(displayName, w);

    setup(page, name);
}

void JsonFieldPage::Field::adjustState(Utils::AbstractMacroExpander *expander)
{
    setVisible(JsonWizard::boolFromVariant(m_visibleExpression, expander));
    setEnabled(JsonWizard::boolFromVariant(m_enabledExpression, expander));
}

void JsonFieldPage::Field::initialize(Utils::AbstractMacroExpander *expander)
{
    adjustState(expander);
    initializeData(expander);
}

// --------------------------------------------------------------------
// JsonFieldPage::LabelFieldData:
// --------------------------------------------------------------------

JsonFieldPage::LabelField::LabelField() :
    m_wordWrap(false)
{ }

bool JsonFieldPage::LabelField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Label data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_wordWrap = tmp.value(QLatin1String("wordWrap"), false).toBool();
    m_text = JsonWizardFactory::localizedString(tmp.value(QLatin1String("trText")));

    if (m_text.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "No text given for Label.");
        return false;
    }

    return true;
}

QWidget *JsonFieldPage::LabelField::widget(const QString &displayName)
{
    Q_UNUSED(displayName);
    QTC_ASSERT(!m_widget, return m_widget);

    QLabel *w = new QLabel();
    w->setWordWrap(m_wordWrap);
    w->setText(m_text);

    m_widget = w;
    return m_widget;
}

// --------------------------------------------------------------------
// JsonFieldPage::SpacerFieldData:
// --------------------------------------------------------------------

JsonFieldPage::SpacerField::SpacerField() :
    m_factor(2)
{ }

bool JsonFieldPage::SpacerField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Spacer data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    bool ok;
    m_factor = tmp.value(QLatin1String("factor"), 2).toInt(&ok);

    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "'size' was not an integer value.");
        return false;
    }

    return true;
}

QWidget *JsonFieldPage::SpacerField::widget(const QString &displayName)
{
    Q_UNUSED(displayName);
    QTC_ASSERT(!m_widget, return m_widget);

    int size = m_widget->style()->layoutSpacing(QSizePolicy::DefaultType, QSizePolicy::DefaultType,
                                                Qt::Vertical) * m_factor;
    m_widget = new QWidget();
    m_widget->setMinimumSize(size, size);
    m_widget->setMaximumSize(size, size);
    m_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return m_widget;
}

// --------------------------------------------------------------------
// JsonFieldPage::LineEditFieldData:
// --------------------------------------------------------------------

JsonFieldPage::LineEditField::LineEditField() :
    m_isModified(false)
{ }

bool JsonFieldPage::LineEditField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "LineEdit data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_defaultText = JsonWizardFactory::localizedString(tmp.value(QLatin1String("trText")).toString());
    m_disabledText = JsonWizardFactory::localizedString(tmp.value(QLatin1String("trDisabledText")).toString());
    m_placeholderText = JsonWizardFactory::localizedString(tmp.value(QLatin1String("trPlaceholder")).toString());

    return true;
}

QWidget *JsonFieldPage::LineEditField::widget(const QString &displayName)
{
    Q_UNUSED(displayName);
    QTC_ASSERT(!m_widget, return m_widget);
    QLineEdit *w = new QLineEdit;
    connect(w, &QLineEdit::textEdited, [this](){ m_isModified = true; });

    m_widget = w;
    return m_widget;
}

void JsonFieldPage::LineEditField::setup(JsonFieldPage *page, const QString &name)
{
    QLineEdit *w = static_cast<QLineEdit *>(m_widget);
    page->registerFieldWithName(name, w);
    connect(w, &QLineEdit::textChanged, page, [page](QString) { page->completeChanged(); });
}

bool JsonFieldPage::LineEditField::validate(Utils::AbstractMacroExpander *expander, QString *message)
{
    Q_UNUSED(message);
    QLineEdit *w = static_cast<QLineEdit *>(m_widget);

    if (!m_isModified)
        w->setText(Utils::expandMacros(m_defaultText, expander));

    if (!w->isEnabled() && !m_disabledText.isNull() && m_currentText.isNull()) {
        m_currentText = w->text();
        w->setText(Utils::expandMacros(m_disabledText, expander));
    } else if (w->isEnabled() && !m_currentText.isNull()) {
        w->setText(m_currentText);
        m_currentText.clear();
    }

    // TODO: Add support for validators
    return !w->text().isEmpty();
}

void JsonFieldPage::LineEditField::initializeData(Utils::AbstractMacroExpander *expander)
{
    QTC_ASSERT(m_widget, return);

    m_isModified = false;

    QLineEdit *w = static_cast<QLineEdit *>(m_widget);
    w->setText(Utils::expandMacros(m_defaultText, expander));
    w->setPlaceholderText(m_placeholderText);
}

// --------------------------------------------------------------------
// JsonFieldPage::TextEditFieldData:
// --------------------------------------------------------------------


JsonFieldPage::TextEditField::TextEditField() :
    m_acceptRichText(false)
{ }

bool JsonFieldPage::TextEditField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "TextEdit data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_defaultText = JsonWizardFactory::localizedString(tmp.value(QLatin1String("trText")).toString());
    m_disabledText = JsonWizardFactory::localizedString(tmp.value(QLatin1String("trDisabledText")).toString());
    m_acceptRichText = tmp.value(QLatin1String("richText"), true).toBool();

    return true;
}

QWidget *JsonFieldPage::TextEditField::widget(const QString &displayName)
{
    // TODO: Set up modification monitoring...
    Q_UNUSED(displayName);
    QTC_ASSERT(!m_widget, return m_widget);
    QTextEdit *w = new QTextEdit;
    w->setAcceptRichText(m_acceptRichText);
    m_widget = w;
    return m_widget;
}

void JsonFieldPage::TextEditField::setup(JsonFieldPage *page, const QString &name)
{
    QTextEdit *w = static_cast<QTextEdit *>(m_widget);
    page->registerFieldWithName(name, w, "plainText", SIGNAL(textChanged()));
    connect(w, &QTextEdit::textChanged, page, &QWizardPage::completeChanged);
}

bool JsonFieldPage::TextEditField::validate(Utils::AbstractMacroExpander *expander, QString *message)
{
    Q_UNUSED(expander);
    Q_UNUSED(message);

    QTextEdit *w = static_cast<QTextEdit *>(m_widget);

    if (!w->isEnabled() && !m_disabledText.isNull() && m_currentText.isNull()) {
        m_currentText = w->toHtml();
        w->setPlainText(Utils::expandMacros(m_disabledText, expander));
    } else if (w->isEnabled() && !m_currentText.isNull()) {
        w->setText(m_currentText);
        m_currentText.clear();
    }

    return !w->toPlainText().isEmpty();
}

void JsonFieldPage::TextEditField::initializeData(Utils::AbstractMacroExpander *expander)
{
    QTextEdit *w = static_cast<QTextEdit *>(m_widget);
    w->setPlainText(Utils::expandMacros(m_defaultText, expander));
}

// --------------------------------------------------------------------
// JsonFieldPage::PathChooserFieldData:
// --------------------------------------------------------------------

JsonFieldPage::PathChooserField::PathChooserField() :
    m_kind(Utils::PathChooser::ExistingDirectory)
{ }

bool JsonFieldPage::PathChooserField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "PathChooser data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_path = tmp.value(QLatin1String("path")).toString();
    m_basePath = tmp.value(QLatin1String("basePath")).toString();

    QString kindStr = tmp.value(QLatin1String("kind"), QLatin1String("existingDirectory")).toString();
    if (kindStr == QLatin1String("existingDirectory")) {
        m_kind = Utils::PathChooser::ExistingDirectory;
    } else if (kindStr == QLatin1String("directory")) {
        m_kind = Utils::PathChooser::Directory;
    } else if (kindStr == QLatin1String("file")) {
        m_kind = Utils::PathChooser::File;
    } else if (kindStr == QLatin1String("saveFile")) {
        m_kind = Utils::PathChooser::SaveFile;
    } else if (kindStr == QLatin1String("existingCommand")) {
        m_kind = Utils::PathChooser::ExistingCommand;
    } else if (kindStr == QLatin1String("command")) {
        m_kind = Utils::PathChooser::Command;
    } else if (kindStr == QLatin1String("any")) {
        m_kind = Utils::PathChooser::Any;
    } else {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "kind '%1' is not one of the supported 'existingDirectory', "
                                                    "'directory', 'file', 'saveFile', 'existingCommand', "
                                                    "'command', 'any'.")
                .arg(kindStr);
        return false;
    }

    return true;
}

QWidget *JsonFieldPage::PathChooserField::widget(const QString &displayName)
{
    Q_UNUSED(displayName);
    QTC_ASSERT(!m_widget, return m_widget);
    m_widget = new Utils::PathChooser;
    return m_widget;
}

void JsonFieldPage::PathChooserField::setEnabled(bool e)
{
    QTC_ASSERT(m_widget, return);
    Utils::PathChooser *w = static_cast<Utils::PathChooser *>(m_widget);
    w->setReadOnly(!e);
}

void JsonFieldPage::PathChooserField::setup(JsonFieldPage *page, const QString &name)
{
    Utils::PathChooser *w = static_cast<Utils::PathChooser *>(m_widget);
    page->registerFieldWithName(name, w, "path", SIGNAL(changed(QString)));
    connect(w, &Utils::PathChooser::changed, page, [page](QString) { page->completeChanged(); });
}

bool JsonFieldPage::PathChooserField::validate(Utils::AbstractMacroExpander *expander, QString *message)
{
    Q_UNUSED(expander);
    Q_UNUSED(message);
    Utils::PathChooser *w = static_cast<Utils::PathChooser *>(m_widget);
    return w->isValid();
}

void JsonFieldPage::PathChooserField::initializeData(Utils::AbstractMacroExpander *expander)
{
    QTC_ASSERT(m_widget, return);
    Utils::PathChooser *w = static_cast<Utils::PathChooser *>(m_widget);
    w->setBaseDirectory(Utils::expandMacros(m_basePath, expander));
    w->setExpectedKind(m_kind);

    if (m_currentPath.isNull())
        w->setPath(Utils::expandMacros(m_path, expander));
    else
        w->setPath(m_currentPath);
}

// --------------------------------------------------------------------
// JsonFieldPage::CheckBoxFieldData:
// --------------------------------------------------------------------

JsonFieldPage::CheckBoxField::CheckBoxField() :
    m_checkedValue(QLatin1String("0")),
    m_uncheckedValue(QLatin1String("1")),
    m_isModified(false)
{ }

bool JsonFieldPage::CheckBoxField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "CheckBox data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_checkedValue = tmp.value(QLatin1String("checkedValue"), QLatin1String("0")).toString();
    m_uncheckedValue = tmp.value(QLatin1String("uncheckedValue"), QLatin1String("1")).toString();
    if (m_checkedValue == m_uncheckedValue) {
        *errorMessage= QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                   "CheckBox values for checked and unchecked state are identical.");
       return false;
    }
    m_checkedExpression = tmp.value(QLatin1String("checked"), false);

    return true;
}

QWidget *JsonFieldPage::CheckBoxField::widget(const QString &displayName)
{
    QTC_ASSERT(!m_widget, return m_widget);
    Utils::TextFieldCheckBox *w = new Utils::TextFieldCheckBox(displayName);
    m_widget = w;
    return m_widget;
}

void JsonFieldPage::CheckBoxField::setup(JsonFieldPage *page, const QString &name)
{
    Utils::TextFieldCheckBox *w = static_cast<Utils::TextFieldCheckBox *>(m_widget);
    connect(w, &Utils::TextFieldCheckBox::clicked, [this]() { m_isModified = true; });
    page->registerFieldWithName(name, w, "text", SIGNAL(textChanged(QString)));
}

bool JsonFieldPage::CheckBoxField::validate(Utils::AbstractMacroExpander *expander, QString *message)
{
    Q_UNUSED(message);
    if (!m_isModified) {
        Utils::TextFieldCheckBox *w = static_cast<Utils::TextFieldCheckBox *>(m_widget);
        w->setChecked(JsonWizard::boolFromVariant(m_checkedExpression, expander));
    }
    return true;
}

void JsonFieldPage::CheckBoxField::initializeData(Utils::AbstractMacroExpander *expander)
{
    QTC_ASSERT(m_widget, return);

    Utils::TextFieldCheckBox *w = static_cast<Utils::TextFieldCheckBox *>(m_widget);
    w->setTrueText(Utils::expandMacros(m_checkedValue, expander));
    w->setFalseText(Utils::expandMacros(m_uncheckedValue, expander));

    w->setChecked(JsonWizard::boolFromVariant(m_checkedExpression, expander));
}

// --------------------------------------------------------------------
// JsonFieldPage::ComboBoxFieldData:
// --------------------------------------------------------------------

JsonFieldPage::ComboBoxField::ComboBoxField() :
    m_index(-1), m_disabledIndex(-1), m_savedIndex(-1), m_currentIndex(-1)
{ }

QPair<QString, QString> parseComboBoxItem(const QVariant &item, QString *errorMessage)
{
    if (item.type() == QVariant::List) {
        *errorMessage  = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                     "No lists allowed inside ComboBox items list.");
        return qMakePair(QString(), QString());
    } else if (item.type() == QVariant::Map) {
        QVariantMap tmp = item.toMap();
        QString key = JsonWizardFactory::localizedString(tmp.value(QLatin1String("trKey"), QString()).toString());
        QString value = tmp.value(QLatin1String("value"), QString()).toString();
        if (key.isNull() || key.isEmpty()) {
            *errorMessage  = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                         "No 'key' found in ComboBox items.");
            return qMakePair(QString(), QString());
        }
        if (value.isNull())
            value = key;
        return qMakePair(key, value);
    } else {
        QString keyvalue = item.toString();
        return qMakePair(keyvalue, keyvalue);
    }
}

bool JsonFieldPage::ComboBoxField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    bool ok;
    m_index = tmp.value(QLatin1String("index"), 0).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox 'index' is not a integer value.");
        return false;
    }
    m_disabledIndex = tmp.value(QLatin1String("disabledIndex"), -1).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox 'disabledIndex' is not a integer value.");
        return false;
    }

    QVariant value = tmp.value(QLatin1String("items"));
    if (value.isNull()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox 'items' missing.");
        return false;
    }
    if (value.type() != QVariant::List) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox 'items' is not a list.");
        return false;
    }

    foreach (const QVariant &i, value.toList()) {
        QPair<QString, QString> keyValue = parseComboBoxItem(i, errorMessage);
        if (keyValue.first.isNull())
            return false; // an error happened...
        m_itemList.append(keyValue.first);
        m_itemDataList.append(keyValue.second);
    }

    return true;
}

QWidget *JsonFieldPage::ComboBoxField::widget(const QString &displayName)
{
    Q_UNUSED(displayName);
    QTC_ASSERT(!m_widget, return m_widget);
    m_widget = new Utils::TextFieldComboBox;
    return m_widget;
}

void JsonFieldPage::ComboBoxField::setup(JsonFieldPage *page, const QString &name)
{
    Utils::TextFieldComboBox *w = static_cast<Utils::TextFieldComboBox *>(m_widget);
    page->registerFieldWithName(name, w, "text", SIGNAL(text4Changed(QString)));
    connect(w, &Utils::TextFieldComboBox::text4Changed, page, [page](QString) { page->completeChanged(); });
}

bool JsonFieldPage::ComboBoxField::validate(Utils::AbstractMacroExpander *expander, QString *message)
{
    Q_UNUSED(expander);
    Q_UNUSED(message);

    Utils::TextFieldComboBox *w = static_cast<Utils::TextFieldComboBox *>(m_widget);
    if (!w->isEnabled() && m_disabledIndex >= 0 && m_savedIndex < 0) {
        m_savedIndex = w->currentIndex();
        w->setCurrentIndex(m_disabledIndex);
    } else if (w->isEnabled() && m_savedIndex >= 0) {
        w->setCurrentIndex(m_savedIndex);
        m_savedIndex = -1;
    }

    return true;
}

void JsonFieldPage::ComboBoxField::initializeData(Utils::AbstractMacroExpander *expander)
{
    Utils::TextFieldComboBox *w = static_cast<Utils::TextFieldComboBox *>(m_widget);
    QStringList tmpItems
            = Utils::transform(m_itemList,
                               [expander](const QString &i) { return Utils::expandMacros(i, expander); });
    QStringList tmpData
            = Utils::transform(m_itemDataList,
                               [expander](const QString &i) { return Utils::expandMacros(i, expander); });
    w->setItems(tmpItems, tmpData);
    w->setInsertPolicy(QComboBox::NoInsert);

    if (m_currentIndex >= 0)
        w->setCurrentIndex(m_currentIndex);
    else
        w->setCurrentIndex(m_index);
}

// --------------------------------------------------------------------
// JsonFieldPage:
// --------------------------------------------------------------------

JsonFieldPage::JsonFieldPage(Utils::AbstractMacroExpander *expander, QWidget *parent) :
    Utils::WizardPage(parent),
    m_formLayout(new QFormLayout),
    m_errorLabel(new QLabel),
    m_expander(expander)
{
    QTC_CHECK(m_expander);

    QVBoxLayout *vLayout = new QVBoxLayout;
    m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    vLayout->addLayout(m_formLayout);
    m_errorLabel->setVisible(false);
    m_errorLabel->setStyleSheet(QLatin1String("background: red"));
    vLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    vLayout->addWidget(m_errorLabel);
    setLayout(vLayout);
}

JsonFieldPage::~JsonFieldPage()
{
    // Do not delete m_expander, it belongs to the wizard!
    qDeleteAll(m_fields);
}

bool JsonFieldPage::setup(const QVariant &data)
{
    QString errorMessage;
    QList<QVariant> fieldList = JsonWizardFactory::objectOrList(data, &errorMessage);
    foreach (const QVariant &field, fieldList) {
        Field *f = JsonFieldPage::Field::parse(field, &errorMessage);
        if (!f)
            continue;
        f->createWidget(this);
        m_fields.append(f);
    }

    return true;
}

bool JsonFieldPage::isComplete() const
{
    QString message;

    bool result = true;
    bool hasErrorMessage = false;
    foreach (Field *f, m_fields) {
        f->adjustState(m_expander);
        if (!f->validate(m_expander, &message)) {
            if (!message.isEmpty()) {
                showError(message);
                hasErrorMessage = true;
            }
            if (f->mandatory)
                result = false;
        }
    }

    if (!hasErrorMessage)
        clearError();

    return result;
}

void JsonFieldPage::initializePage()
{
    foreach (Field *f, m_fields)
        f->initialize(m_expander);
}

void JsonFieldPage::cleanupPage()
{
    foreach (Field *f, m_fields)
        f->cleanup(m_expander);
}

void JsonFieldPage::showError(const QString &m) const
{
    m_errorLabel->setText(m);
    m_errorLabel->setVisible(true);
}

void JsonFieldPage::clearError() const
{
    m_errorLabel->setText(QString());
    m_errorLabel->setVisible(false);
}

Utils::AbstractMacroExpander *JsonFieldPage::expander()
{
    return m_expander;
}

} // namespace ProjectExplorer
