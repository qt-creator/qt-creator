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

#include "jsonfieldpage.h"

#include "jsonwizard.h"
#include "jsonwizardfactory.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/textfieldcheckbox.h>
#include <utils/textfieldcombobox.h>

#include <QCheckBox>
#include <QApplication>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QTextEdit>
#include <QVariant>
#include <QVariantMap>
#include <QVBoxLayout>

using namespace Utils;

const char NAME_KEY[] = "name";
const char DISPLAY_NAME_KEY[] = "trDisplayName";
const char MANDATORY_KEY[] = "mandatory";
const char VISIBLE_KEY[] = "visible";
const char ENABLED_KEY[] = "enabled";
const char SPAN_KEY[] = "span";
const char TYPE_KEY[] = "type";
const char DATA_KEY[] = "data";

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

class LineEditValidator : public QRegularExpressionValidator
{
public:
    LineEditValidator(MacroExpander *expander, const QRegularExpression &pattern, QObject *parent) :
        QRegularExpressionValidator(pattern, parent)
    {
        m_expander.setDisplayName(JsonFieldPage::tr("Line Edit Validator Expander"));
        m_expander.setAccumulating(true);
        m_expander.registerVariable("INPUT", JsonFieldPage::tr("The text edit input to fix up."),
                                    [this]() { return m_currentInput; });
        m_expander.registerSubProvider([expander]() -> MacroExpander * { return expander; });
    }

    void setFixupExpando(const QString &expando)
    {
        m_fixupExpando = expando;
    }

    QValidator::State validate(QString &input, int &pos) const
    {
        fixup(input);
        return QRegularExpressionValidator::validate(input, pos);
    }

    void fixup(QString &fixup) const
    {
        if (m_fixupExpando.isEmpty())
            return;

        m_currentInput = fixup;
        fixup = m_expander.expand(m_fixupExpando);
    }

private:
    MacroExpander m_expander;
    QString m_fixupExpando;
    mutable QString m_currentInput;
};

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
                                                    "Field \"%1\" has no type.").arg(name);
        return 0;
    }

    Field *data = createFieldData(type);
    if (!data) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field \"%1\" has unsupported type \"%2\".")
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
                                                    "When parsing Field \"%1\": %2")
                .arg(name).arg(*errorMessage);
        delete data;
        return 0;
    }

    return data;
}

void JsonFieldPage::Field::createWidget(JsonFieldPage *page)
{
    QWidget *w = widget(displayName, page);
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

void JsonFieldPage::Field::adjustState(MacroExpander *expander)
{
    setVisible(JsonWizard::boolFromVariant(m_visibleExpression, expander));
    setEnabled(JsonWizard::boolFromVariant(m_enabledExpression, expander));
}

void JsonFieldPage::Field::initialize(MacroExpander *expander)
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

QWidget *JsonFieldPage::LabelField::widget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
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
    m_factor(1)
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
    m_factor = tmp.value(QLatin1String("factor"), 1).toInt(&ok);

    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "\"factor\" is no integer value.");
        return false;
    }

    return true;
}

QWidget *JsonFieldPage::SpacerField::widget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    QTC_ASSERT(!m_widget, return m_widget);

    int size = qApp->style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing) * m_factor;

    m_widget = new QWidget();
    m_widget->setMinimumSize(size, size);
    m_widget->setMaximumSize(size, size);
    m_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return m_widget;
}

// --------------------------------------------------------------------
// JsonFieldPage::LineEditFieldData:
// --------------------------------------------------------------------

JsonFieldPage::LineEditField::LineEditField() : m_isModified(false), m_isValidating(false)
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
    QString pattern = tmp.value(QLatin1String("validator")).toString();
    if (!pattern.isEmpty()) {
        m_validatorRegExp = QRegularExpression(pattern);
        if (!m_validatorRegExp.isValid()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                        "Invalid regular expression \"%1\" in \"validator\".")
                    .arg(pattern);
            m_validatorRegExp = QRegularExpression();
            return false;
        }
    }
    m_fixupExpando = tmp.value(QLatin1String("fixup")).toString();

    return true;
}

QWidget *JsonFieldPage::LineEditField::widget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    QTC_ASSERT(!m_widget, return m_widget);
    QLineEdit *w = new QLineEdit;

    if (m_validatorRegExp.isValid()) {
        LineEditValidator *lv = new LineEditValidator(page->expander(), m_validatorRegExp, w);
        lv->setFixupExpando(m_fixupExpando);
        w->setValidator(lv);
    }

    m_widget = w;
    return m_widget;
}

void JsonFieldPage::LineEditField::setup(JsonFieldPage *page, const QString &name)
{
    QLineEdit *w = static_cast<QLineEdit *>(m_widget);
    page->registerFieldWithName(name, w);
    connect(w, &QLineEdit::textChanged,
            page, [this, page]() -> void { m_isModified = true; emit page->completeChanged(); });
}

bool JsonFieldPage::LineEditField::validate(MacroExpander *expander, QString *message)
{
    Q_UNUSED(message);
    if (m_isValidating)
        return true;

    m_isValidating = true;

    QLineEdit *w = static_cast<QLineEdit *>(m_widget);

    if (w->isEnabled()) {
        if (m_isModified) {
            if (!m_currentText.isNull()) {
                w->setText(m_currentText);
                m_currentText.clear();
            }
        } else {
            w->setText(expander->expand(m_defaultText));
            m_isModified = false;
        }
    } else {
        if (!m_disabledText.isNull() && m_currentText.isNull())
            m_currentText = w->text();
    }

    m_isValidating = false;

    return !w->text().isEmpty();
}

void JsonFieldPage::LineEditField::initializeData(MacroExpander *expander)
{
    QTC_ASSERT(m_widget, return);

    QLineEdit *w = static_cast<QLineEdit *>(m_widget);
    m_isValidating = true;
    w->setText(expander->expand(m_defaultText));
    w->setPlaceholderText(m_placeholderText);
    m_isModified = false;
    m_isValidating = false;
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

QWidget *JsonFieldPage::TextEditField::widget(const QString &displayName, JsonFieldPage *page)
{
    // TODO: Set up modification monitoring...
    Q_UNUSED(displayName);
    Q_UNUSED(page);
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

bool JsonFieldPage::TextEditField::validate(MacroExpander *expander, QString *message)
{
    Q_UNUSED(expander);
    Q_UNUSED(message);

    QTextEdit *w = static_cast<QTextEdit *>(m_widget);

    if (!w->isEnabled() && !m_disabledText.isNull() && m_currentText.isNull()) {
        m_currentText = w->toHtml();
        w->setPlainText(expander->expand(m_disabledText));
    } else if (w->isEnabled() && !m_currentText.isNull()) {
        w->setText(m_currentText);
        m_currentText.clear();
    }

    return !w->toPlainText().isEmpty();
}

void JsonFieldPage::TextEditField::initializeData(MacroExpander *expander)
{
    QTextEdit *w = static_cast<QTextEdit *>(m_widget);
    w->setPlainText(expander->expand(m_defaultText));
}

// --------------------------------------------------------------------
// JsonFieldPage::PathChooserFieldData:
// --------------------------------------------------------------------

JsonFieldPage::PathChooserField::PathChooserField() :
    m_kind(PathChooser::ExistingDirectory)
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
        m_kind = PathChooser::ExistingDirectory;
    } else if (kindStr == QLatin1String("directory")) {
        m_kind = PathChooser::Directory;
    } else if (kindStr == QLatin1String("file")) {
        m_kind = PathChooser::File;
    } else if (kindStr == QLatin1String("saveFile")) {
        m_kind = PathChooser::SaveFile;
    } else if (kindStr == QLatin1String("existingCommand")) {
        m_kind = PathChooser::ExistingCommand;
    } else if (kindStr == QLatin1String("command")) {
        m_kind = PathChooser::Command;
    } else if (kindStr == QLatin1String("any")) {
        m_kind = PathChooser::Any;
    } else {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "kind \"%1\" is not one of the supported \"existingDirectory\", "
                                                    "\"directory\", \"file\", \"saveFile\", \"existingCommand\", "
                                                    "\"command\", \"any\".")
                .arg(kindStr);
        return false;
    }

    return true;
}

QWidget *JsonFieldPage::PathChooserField::widget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    QTC_ASSERT(!m_widget, return m_widget);
    m_widget = new PathChooser;
    return m_widget;
}

void JsonFieldPage::PathChooserField::setEnabled(bool e)
{
    QTC_ASSERT(m_widget, return);
    PathChooser *w = static_cast<PathChooser *>(m_widget);
    w->setReadOnly(!e);
}

void JsonFieldPage::PathChooserField::setup(JsonFieldPage *page, const QString &name)
{
    PathChooser *w = static_cast<PathChooser *>(m_widget);
    page->registerFieldWithName(name, w, "path", SIGNAL(changed(QString)));
    connect(w, &PathChooser::changed, page, [page](QString) { page->completeChanged(); });
}

bool JsonFieldPage::PathChooserField::validate(MacroExpander *expander, QString *message)
{
    Q_UNUSED(expander);
    Q_UNUSED(message);
    PathChooser *w = static_cast<PathChooser *>(m_widget);
    return w->isValid();
}

void JsonFieldPage::PathChooserField::initializeData(MacroExpander *expander)
{
    QTC_ASSERT(m_widget, return);
    PathChooser *w = static_cast<PathChooser *>(m_widget);
    w->setBaseDirectory(expander->expand(m_basePath));
    w->setExpectedKind(m_kind);

    if (m_currentPath.isNull())
        w->setPath(expander->expand(m_path));
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

QWidget *JsonFieldPage::CheckBoxField::widget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(page);
    QTC_ASSERT(!m_widget, return m_widget);
    TextFieldCheckBox *w = new TextFieldCheckBox(displayName);
    m_widget = w;
    return m_widget;
}

void JsonFieldPage::CheckBoxField::setup(JsonFieldPage *page, const QString &name)
{
    TextFieldCheckBox *w = static_cast<TextFieldCheckBox *>(m_widget);
    connect(w, &TextFieldCheckBox::clicked, [this]() { m_isModified = true; });
    page->registerFieldWithName(name, w, "text", SIGNAL(textChanged(QString)));
}

bool JsonFieldPage::CheckBoxField::validate(MacroExpander *expander, QString *message)
{
    Q_UNUSED(message);
    if (!m_isModified) {
        TextFieldCheckBox *w = static_cast<TextFieldCheckBox *>(m_widget);
        w->setChecked(JsonWizard::boolFromVariant(m_checkedExpression, expander));
    }
    return true;
}

void JsonFieldPage::CheckBoxField::initializeData(MacroExpander *expander)
{
    QTC_ASSERT(m_widget, return);

    TextFieldCheckBox *w = static_cast<TextFieldCheckBox *>(m_widget);
    w->setTrueText(expander->expand(m_checkedValue));
    w->setFalseText(expander->expand(m_uncheckedValue));

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
                                                         "No \"key\" found in ComboBox items.");
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
                                                    "ComboBox \"index\" is not an integer value.");
        return false;
    }
    m_disabledIndex = tmp.value(QLatin1String("disabledIndex"), -1).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox \"disabledIndex\" is not an integer value.");
        return false;
    }

    QVariant value = tmp.value(QLatin1String("items"));
    if (value.isNull()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox \"items\" missing.");
        return false;
    }
    if (value.type() != QVariant::List) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox \"items\" is not a list.");
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

QWidget *JsonFieldPage::ComboBoxField::widget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    QTC_ASSERT(!m_widget, return m_widget);
    m_widget = new TextFieldComboBox;
    return m_widget;
}

void JsonFieldPage::ComboBoxField::setup(JsonFieldPage *page, const QString &name)
{
    TextFieldComboBox *w = static_cast<TextFieldComboBox *>(m_widget);
    page->registerFieldWithName(name, w, "text", SIGNAL(text4Changed(QString)));
    connect(w, &TextFieldComboBox::text4Changed, page, [page](QString) { page->completeChanged(); });
}

bool JsonFieldPage::ComboBoxField::validate(MacroExpander *expander, QString *message)
{
    Q_UNUSED(expander);
    Q_UNUSED(message);

    TextFieldComboBox *w = static_cast<TextFieldComboBox *>(m_widget);
    if (!w->isEnabled() && m_disabledIndex >= 0 && m_savedIndex < 0) {
        m_savedIndex = w->currentIndex();
        w->setCurrentIndex(m_disabledIndex);
    } else if (w->isEnabled() && m_savedIndex >= 0) {
        w->setCurrentIndex(m_savedIndex);
        m_savedIndex = -1;
    }

    return true;
}

void JsonFieldPage::ComboBoxField::initializeData(MacroExpander *expander)
{
    TextFieldComboBox *w = static_cast<TextFieldComboBox *>(m_widget);
    QStringList tmpItems
            = Utils::transform(m_itemList,
                               [expander](const QString &i) { return expander->expand(i); });
    QStringList tmpData
            = Utils::transform(m_itemDataList,
                               [expander](const QString &i) { return expander->expand(i); });
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

JsonFieldPage::JsonFieldPage(MacroExpander *expander, QWidget *parent) :
    WizardPage(parent),
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

MacroExpander *JsonFieldPage::expander()
{
    return m_expander;
}

} // namespace ProjectExplorer
