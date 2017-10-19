/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "jsonfieldpage.h"
#include "jsonfieldpage_p.h"

#include "jsonwizard.h"
#include "jsonwizardfactory.h"

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/textfieldcombobox.h>
#include <utils/theme/theme.h>

#include <QCheckBox>
#include <QApplication>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QTextEdit>
#include <QVariant>
#include <QVariantMap>
#include <QVBoxLayout>

using namespace Utils;

const char NAME_KEY[] = "name";
const char DISPLAY_NAME_KEY[] = "trDisplayName";
const char TOOLTIP_KEY[] = "trToolTip";
const char MANDATORY_KEY[] = "mandatory";
const char VISIBLE_KEY[] = "visible";
const char ENABLED_KEY[] = "enabled";
const char SPAN_KEY[] = "span";
const char TYPE_KEY[] = "type";
const char DATA_KEY[] = "data";
const char IS_COMPLETE_KEY[] = "isComplete";
const char IS_COMPLETE_MESSAGE_KEY[] = "trIncompleteMessage";

namespace {
QVariant consumeValue(QVariantMap &map, const QString &key, const QVariant &defaultValue = QVariant())
{
    QVariantMap::iterator i = map.find(key);
    if (i != map.end()) {
        QVariant value = i.value();
        map.erase(i);
        return value;
    }
    return defaultValue;
}

void warnAboutUnsupportedKeys(const QVariantMap &map, const QString &name, const QString &type = QString())
{
    if (!map.isEmpty()) {

        QString typeAndName = name;
        if (!type.isEmpty() && !name.isEmpty())
            typeAndName = QString("%1(\"%2\")").arg(type, name);

        qWarning().noquote() << QString("Field %1 has unsupported keys: %2").arg(typeAndName, map.keys().join(", "));
    }
}
} // namespace

namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

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

JsonFieldPage::Field::Field() : d(new FieldPrivate)
{ }

JsonFieldPage::Field::~Field()
{
    delete d->m_widget;
    delete d->m_label;
    delete d;
}

QString JsonFieldPage::Field::type()
{
    return d->m_type;
}

JsonFieldPage::Field *JsonFieldPage::Field::parse(const QVariant &input, QString *errorMessage)
{
    if (input.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field is not an object.");
        return 0;
    }

    QVariantMap tmp = input.toMap();
    const QString name = consumeValue(tmp, NAME_KEY).toString();
    if (name.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field has no name.");
        return 0;
    }
    const QString type = consumeValue(tmp, TYPE_KEY).toString();
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
    data->setTexts(name,
                   JsonWizardFactory::localizedString(consumeValue(tmp, DISPLAY_NAME_KEY).toString()),
                   consumeValue(tmp, TOOLTIP_KEY).toString());

    data->setVisibleExpression(consumeValue(tmp, VISIBLE_KEY, true));
    data->setEnabledExpression(consumeValue(tmp, ENABLED_KEY, true));
    data->setIsMandatory(consumeValue(tmp, MANDATORY_KEY, true).toBool());
    data->setHasSpan(consumeValue(tmp, SPAN_KEY, false).toBool());
    data->setIsCompleteExpando(consumeValue(tmp, IS_COMPLETE_KEY, true),
                               consumeValue(tmp, IS_COMPLETE_MESSAGE_KEY).toString());

    QVariant dataVal = consumeValue(tmp, DATA_KEY);
    if (!data->parseData(dataVal, errorMessage)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "When parsing Field \"%1\": %2")
                .arg(name).arg(*errorMessage);
        delete data;
        return 0;
    }

    warnAboutUnsupportedKeys(tmp, name);
    return data;
}

void JsonFieldPage::Field::createWidget(JsonFieldPage *page)
{
    QWidget *w = widget(displayName(), page);
    w->setObjectName(name());
    QFormLayout *layout = page->layout();

    if (suppressName()) {
        layout->addWidget(w);
    } else if (hasSpan()) {
        if (!suppressName()) {
            d->m_label = new QLabel(displayName());
            layout->addRow(d->m_label);
        }

        layout->addRow(w);
    } else {
        d->m_label = new QLabel(displayName());
        layout->addRow(d->m_label, w);
    }

    setup(page, name());
}

void JsonFieldPage::Field::adjustState(MacroExpander *expander)
{
    setVisible(JsonWizard::boolFromVariant(d->m_visibleExpression, expander));
    setEnabled(JsonWizard::boolFromVariant(d->m_enabledExpression, expander));
    QTC_ASSERT(d->m_widget, return);
    d->m_widget->setToolTip(expander->expand(toolTip()));
}

void JsonFieldPage::Field::setEnabled(bool e)
{
    QTC_ASSERT(d->m_widget, return);
    d->m_widget->setEnabled(e);
}

void JsonFieldPage::Field::setVisible(bool v)
{
    QTC_ASSERT(d->m_widget, return);
    if (d->m_label)
        d->m_label->setVisible(v);
    d->m_widget->setVisible(v);
}

void JsonFieldPage::Field::setType(const QString &type)
{
    d->m_type = type;
}

bool JsonFieldPage::Field::validate(MacroExpander *expander, QString *message)
{
    if (!JsonWizard::boolFromVariant(d->m_isCompleteExpando, expander)) {
        if (message)
            *message = expander->expand(d->m_isCompleteExpandoMessage);
        return false;
    }
    return true;
}

void JsonFieldPage::Field::initialize(MacroExpander *expander)
{
    adjustState(expander);
    initializeData(expander);
}

QWidget *JsonFieldPage::Field::widget(const QString &displayName, JsonFieldPage *page)
{
    QTC_ASSERT(!d->m_widget, return d->m_widget);

    d->m_widget = createWidget(displayName, page);
    return d->m_widget;
}

QString JsonFieldPage::Field::name()
{
    return d->m_name;
}

QString JsonFieldPage::Field::displayName()
{
    return d->m_displayName;
}

QString JsonFieldPage::Field::toolTip()
{
    return d->m_toolTip;
}

bool JsonFieldPage::Field::isMandatory()
{
    return d->m_isMandatory;
}

bool JsonFieldPage::Field::hasSpan()
{
    return d->m_hasSpan;
}

QVariant JsonFieldPage::value(const QString &key)
{
    QVariant v = property(key.toUtf8());
    if (v.isValid())
        return v;
    auto w = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(w, return QVariant());
    return w->value(key);
}

QWidget *JsonFieldPage::Field::widget() const
{
    return d->m_widget;
}

void JsonFieldPage::Field::setTexts(const QString &n, const QString &dn, const QString &tt)
{
    d->m_name = n;
    d->m_displayName = dn;
    d->m_toolTip = tt;
}

void JsonFieldPage::Field::setIsMandatory(bool b)
{
    d->m_isMandatory = b;
}

void JsonFieldPage::Field::setHasSpan(bool b)
{
    d->m_hasSpan = b;
}

void JsonFieldPage::Field::setVisibleExpression(const QVariant &v)
{
    d->m_visibleExpression = v;
}

void JsonFieldPage::Field::setEnabledExpression(const QVariant &v)
{
    d->m_enabledExpression = v;
}

void JsonFieldPage::Field::setIsCompleteExpando(const QVariant &v, const QString &m)
{
    d->m_isCompleteExpando = v;
    d->m_isCompleteExpandoMessage = m;
}

// --------------------------------------------------------------------
// LabelFieldData:
// --------------------------------------------------------------------

bool LabelField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Label(\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_wordWrap = consumeValue(tmp, "wordWrap", false).toBool();
    m_text = JsonWizardFactory::localizedString(consumeValue(tmp, "trText"));

    if (m_text.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Label(\"%1\") has no trText.")
                .arg(name());
        return false;
    }
    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}

QWidget *LabelField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    auto w = new QLabel;
    w->setWordWrap(m_wordWrap);
    w->setText(m_text);
    return w;
}

// --------------------------------------------------------------------
// SpacerFieldData:
// --------------------------------------------------------------------

bool SpacerField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Spacer(\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    bool ok;
    m_factor = consumeValue(tmp, "factor", 1).toInt(&ok);

    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Spacer(\"%1\") property \"factor\" is no integer value.")
                .arg(name());
        return false;
    }
    warnAboutUnsupportedKeys(tmp, name(), type());

    return true;
}

QWidget *SpacerField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    int size = QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing) * m_factor;

    auto w = new QWidget();
    w->setMinimumSize(size, size);
    w->setMaximumSize(size, size);
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return w;
}

// --------------------------------------------------------------------
// LineEditFieldData:
// --------------------------------------------------------------------

bool LineEditField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "LineEdit(\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_isPassword = consumeValue(tmp, "isPassword", false).toBool();
    m_defaultText = JsonWizardFactory::localizedString(consumeValue(tmp, "trText").toString());
    m_disabledText = JsonWizardFactory::localizedString(consumeValue(tmp, "trDisabledText").toString());
    m_placeholderText = JsonWizardFactory::localizedString(consumeValue(tmp, "trPlaceholder").toString());
    m_historyId = consumeValue(tmp, "historyId").toString();
    m_restoreLastHistoryItem = consumeValue(tmp, "restoreLastHistoyItem", false).toBool();
    QString pattern = consumeValue(tmp, "validator").toString();
    if (!pattern.isEmpty()) {
        m_validatorRegExp = QRegularExpression(pattern);
        if (!m_validatorRegExp.isValid()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                        "LineEdit(\"%1\") has an invalid regular expression \"%1\" in \"validator\".")
                    .arg(name(), pattern);
            m_validatorRegExp = QRegularExpression();
            return false;
        }
    }
    m_fixupExpando = consumeValue(tmp, "fixup").toString();

    warnAboutUnsupportedKeys(tmp, name(), type());

    return true;
}

QWidget *LineEditField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    auto w = new FancyLineEdit;

    if (m_validatorRegExp.isValid()) {
        auto lv = new LineEditValidator(page->expander(), m_validatorRegExp, w);
        lv->setFixupExpando(m_fixupExpando);
        w->setValidator(lv);
    }

    if (!m_historyId.isEmpty())
        w->setHistoryCompleter(m_historyId, m_restoreLastHistoryItem);

    w->setEchoMode(m_isPassword ? QLineEdit::Password : QLineEdit::Normal);

    return w;
}

void LineEditField::setup(JsonFieldPage *page, const QString &name)
{
    auto w = qobject_cast<FancyLineEdit *>(widget());
    QTC_ASSERT(w, return);
    page->registerFieldWithName(name, w);
    QObject::connect(w, &FancyLineEdit::textChanged,
                     page, [this, page]() -> void { m_isModified = true; emit page->completeChanged(); });
}

bool LineEditField::validate(MacroExpander *expander, QString *message)
{
    if (!JsonFieldPage::Field::validate(expander, message))
        return false;

    if (m_isValidating)
        return true;

    m_isValidating = true;

    auto w = qobject_cast<FancyLineEdit *>(widget());
    QTC_ASSERT(w, return false);

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

void LineEditField::initializeData(MacroExpander *expander)
{
    auto w = qobject_cast<FancyLineEdit *>(widget());
    QTC_ASSERT(w, return);
    m_isValidating = true;
    w->setText(expander->expand(m_defaultText));
    w->setPlaceholderText(m_placeholderText);
    m_isModified = false;
    m_isValidating = false;
}

// --------------------------------------------------------------------
// TextEditFieldData:
// --------------------------------------------------------------------


bool TextEditField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "TextEdit(\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_defaultText = JsonWizardFactory::localizedString(consumeValue(tmp, "trText").toString());
    m_disabledText = JsonWizardFactory::localizedString(consumeValue(tmp, "trDisabledText").toString());
    m_acceptRichText = consumeValue(tmp, "richText", true).toBool();

    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}

QWidget *TextEditField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    // TODO: Set up modification monitoring...
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    auto w = new QTextEdit;
    w->setAcceptRichText(m_acceptRichText);
    return w;
}

void TextEditField::setup(JsonFieldPage *page, const QString &name)
{
    auto w = qobject_cast<QTextEdit *>(widget());
    QTC_ASSERT(w, return);
    page->registerFieldWithName(name, w, "plainText", SIGNAL(textChanged()));
    QObject::connect(w, &QTextEdit::textChanged, page, &QWizardPage::completeChanged);
}

bool TextEditField::validate(MacroExpander *expander, QString *message)
{
    if (!JsonFieldPage::Field::validate(expander, message))
        return false;

    auto w = qobject_cast<QTextEdit *>(widget());
    QTC_ASSERT(w, return false);

    if (!w->isEnabled() && !m_disabledText.isNull() && m_currentText.isNull()) {
        m_currentText = w->toHtml();
        w->setPlainText(expander->expand(m_disabledText));
    } else if (w->isEnabled() && !m_currentText.isNull()) {
        w->setText(m_currentText);
        m_currentText.clear();
    }

    return !w->toPlainText().isEmpty();
}

void TextEditField::initializeData(MacroExpander *expander)
{
    auto w = qobject_cast<QTextEdit *>(widget());
    QTC_ASSERT(w, return);
    w->setPlainText(expander->expand(m_defaultText));
}

// --------------------------------------------------------------------
// PathChooserFieldData:
// --------------------------------------------------------------------

bool PathChooserField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "PathChooser data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_path = consumeValue(tmp, "path").toString();
    m_basePath = consumeValue(tmp, "basePath").toString();
    m_historyId = consumeValue(tmp, "historyId").toString();

    QString kindStr = consumeValue(tmp, "kind", "existingDirectory").toString();
    if (kindStr == "existingDirectory") {
        m_kind = PathChooser::ExistingDirectory;
    } else if (kindStr == "directory") {
        m_kind = PathChooser::Directory;
    } else if (kindStr == "file") {
        m_kind = PathChooser::File;
    } else if (kindStr == "saveFile") {
        m_kind = PathChooser::SaveFile;
    } else if (kindStr == "existingCommand") {
        m_kind = PathChooser::ExistingCommand;
    } else if (kindStr == "command") {
        m_kind = PathChooser::Command;
    } else if (kindStr == "any") {
        m_kind = PathChooser::Any;
    } else {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "kind \"%1\" is not one of the supported \"existingDirectory\", "
                                                    "\"directory\", \"file\", \"saveFile\", \"existingCommand\", "
                                                    "\"command\", \"any\".")
                .arg(kindStr);
        return false;
    }

    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}

QWidget *PathChooserField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    auto w = new PathChooser;
    if (!m_historyId.isEmpty())
        w->setHistoryCompleter(m_historyId);
    return w;
}

void PathChooserField::setEnabled(bool e)
{
    auto w = qobject_cast<PathChooser *>(widget());
    QTC_ASSERT(w, return);
    w->setReadOnly(!e);
}

void PathChooserField::setup(JsonFieldPage *page, const QString &name)
{
    auto w = qobject_cast<PathChooser *>(widget());
    QTC_ASSERT(w, return);
    page->registerFieldWithName(name, w, "path", SIGNAL(rawPathChanged(QString)));
    QObject::connect(w, &PathChooser::rawPathChanged,
                     page, [page](QString) { page->completeChanged(); });
}

bool PathChooserField::validate(MacroExpander *expander, QString *message)
{
    if (!JsonFieldPage::Field::validate(expander, message))
        return false;

    auto w = qobject_cast<PathChooser *>(widget());
    QTC_ASSERT(w, return false);
    return w->isValid();
}

void PathChooserField::initializeData(MacroExpander *expander)
{
    auto w = qobject_cast<PathChooser *>(widget());
    QTC_ASSERT(w, return);
    w->setBaseDirectory(expander->expand(m_basePath));
    w->setExpectedKind(m_kind);

    if (m_currentPath.isNull())
        w->setPath(expander->expand(m_path));
    else
        w->setPath(m_currentPath);
}

// --------------------------------------------------------------------
// CheckBoxFieldData:
// --------------------------------------------------------------------

bool CheckBoxField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return true;

    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "CheckBox(\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_checkedValue = consumeValue(tmp, "checkedValue", true).toString();
    m_uncheckedValue = consumeValue(tmp, "uncheckedValue", false).toString();
    if (m_checkedValue == m_uncheckedValue) {
        *errorMessage= QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                   "CheckBox(\"%1\") values for checked and unchecked state are identical.")
                .arg(name());
       return false;
    }
    m_checkedExpression = consumeValue(tmp, "checked", false);

    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}

QWidget *CheckBoxField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(page);
    return new QCheckBox(displayName);
}

void CheckBoxField::setup(JsonFieldPage *page, const QString &name)
{
    auto w = qobject_cast<QCheckBox *>(widget());
    QTC_ASSERT(w, return);
    page->registerObjectAsFieldWithName<QCheckBox>(name, w, &QCheckBox::stateChanged, [this, page, w] () -> QString {
        if (w->checkState() == Qt::Checked)
            return page->expander()->expand(m_checkedValue);
        return page->expander()->expand(m_uncheckedValue);
    });

    QObject::connect(w, &QCheckBox::stateChanged, page, [this, page]() {
        m_isModified = true;
        emit page->completeChanged();
    });
}

bool CheckBoxField::validate(MacroExpander *expander, QString *message)
{
    if (!JsonFieldPage::Field::validate(expander, message))
        return false;

    if (!m_isModified) {
        auto w = qobject_cast<QCheckBox *>(widget());
        QTC_ASSERT(w, return false);
        w->setChecked(JsonWizard::boolFromVariant(m_checkedExpression, expander));
    }
    return true;
}

void CheckBoxField::initializeData(MacroExpander *expander)
{
    auto w = qobject_cast<QCheckBox *>(widget());
    QTC_ASSERT(widget(), return);

    w->setChecked(JsonWizard::boolFromVariant(m_checkedExpression, expander));
}

// --------------------------------------------------------------------
// ComboBoxFieldData:
// --------------------------------------------------------------------

struct ComboBoxItem {
    ComboBoxItem(const QString &k = QString(), const QString &v = QString(), const QVariant &c = true) :
        key(k), value(v), condition(c)
    { }

    QString key;
    QString value;
    QVariant condition;
};

ComboBoxItem parseComboBoxItem(const QVariant &item, QString *errorMessage)
{
    if (item.type() == QVariant::List) {
        *errorMessage  = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                     "No lists allowed inside ComboBox items list.");
        return ComboBoxItem();
    } else if (item.type() == QVariant::Map) {
        QVariantMap tmp = item.toMap();
        QString key = JsonWizardFactory::localizedString(consumeValue(tmp, QLatin1String("trKey"), QString()).toString());
        QString value = consumeValue(tmp, QLatin1String("value"), QString()).toString();
        QVariant condition = consumeValue(tmp, QLatin1String("condition"), true);
        if (key.isNull() || key.isEmpty()) {
            *errorMessage  = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                         "No \"key\" found in ComboBox items.");
            return ComboBoxItem();
        }
        if (value.isNull())
            value = key;
        return ComboBoxItem(key, value, condition);
    } else {
        QString keyvalue = item.toString();
        return ComboBoxItem(keyvalue, keyvalue);
    }
}

bool ComboBoxField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox data is not an object.");
        return false;
    }

    QVariantMap tmp = data.toMap();

    bool ok;
    m_index = consumeValue(tmp, "index", 0).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox(\"%1\") \"index\" is not an integer value.")
                .arg(name());
        return false;
    }
    m_disabledIndex = consumeValue(tmp, "disabledIndex", -1).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox(\"%1\") \"disabledIndex\" is not an integer value.")
                .arg(name());
        return false;
    }

    QVariant value = consumeValue(tmp, "items");
    if (value.isNull()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox(\"%1\") \"items\" missing.")
                .arg(name());
        return false;
    }
    if (value.type() != QVariant::List) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "ComboBox(\"%1\") \"items\" is not a list.")
                .arg(name());
        return false;
    }

    foreach (const QVariant &i, value.toList()) {
        ComboBoxItem keyValue = parseComboBoxItem(i, errorMessage);
        if (keyValue.key.isNull())
            return false; // an error happened...
        m_itemList.append(keyValue.key);
        m_itemDataList.append(keyValue.value);
        m_itemConditionList.append(keyValue.condition);
    }

    if (m_itemConditionList.count() != m_itemDataList.count()
            || m_itemConditionList.count() != m_itemList.count()) {
        m_itemConditionList.clear();
        m_itemDataList.clear();
        m_itemList.clear();
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Internal Error: ComboBox(\"%1\") items lists got mixed up.")
                .arg(name());
        return false;
    }
    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}

QWidget *ComboBoxField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName);
    Q_UNUSED(page);
    return new TextFieldComboBox;
}

void ComboBoxField::setup(JsonFieldPage *page, const QString &name)
{
    auto w = qobject_cast<TextFieldComboBox *>(widget());
    QTC_ASSERT(w, return);
    page->registerFieldWithName(name, w, "indexText", SIGNAL(text4Changed(QString)));
    QObject::connect(w, &TextFieldComboBox::text4Changed,
                     page, [page](QString) { page->completeChanged(); });
}

bool ComboBoxField::validate(MacroExpander *expander, QString *message)
{
    if (!JsonFieldPage::Field::validate(expander, message))
        return false;

    auto w = qobject_cast<TextFieldComboBox *>(widget());
    QTC_ASSERT(w, return false);
    if (!w->isEnabled() && m_disabledIndex >= 0 && m_savedIndex < 0) {
        m_savedIndex = w->currentIndex();
        w->setCurrentIndex(m_disabledIndex);
    } else if (w->isEnabled() && m_savedIndex >= 0) {
        w->setCurrentIndex(m_savedIndex);
        m_savedIndex = -1;
    }

    return true;
}

void ComboBoxField::initializeData(MacroExpander *expander)
{
    auto w = qobject_cast<TextFieldComboBox *>(widget());
    QTC_ASSERT(widget(), return);
    QStringList tmpItems
            = Utils::transform(m_itemList,
                               [expander](const QString &i) { return expander->expand(i); });
    QStringList tmpData
            = Utils::transform(m_itemDataList,
                               [expander](const QString &i) { return expander->expand(i); });
    QList<bool> tmpConditions
            = Utils::transform(m_itemConditionList,
                               [expander](const QVariant &v) { return JsonWizard::boolFromVariant(v, expander); });

    int index = m_index;
    for (int i = tmpConditions.count() - 1; i >= 0; --i) {
        if (!tmpConditions.at(i)) {
            tmpItems.removeAt(i);
            tmpData.removeAt(i);
            if (i < index && index > 0)
                --index;
        }
    }

    if (index < 0 || index >= tmpData.count())
        index = 0;
    w->setItems(tmpItems, tmpData);
    w->setInsertPolicy(QComboBox::NoInsert);
    w->setCurrentIndex(index);
}

// --------------------------------------------------------------------
// JsonFieldPage:
// --------------------------------------------------------------------

QHash<QString, JsonFieldPage::FieldFactory> JsonFieldPage::m_factories;

JsonFieldPage::JsonFieldPage(MacroExpander *expander, QWidget *parent) :
    WizardPage(parent),
    m_formLayout(new QFormLayout),
    m_errorLabel(new QLabel),
    m_expander(expander)
{
    QTC_CHECK(m_expander);

    auto vLayout = new QVBoxLayout;
    m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    vLayout->addLayout(m_formLayout);
    m_errorLabel->setVisible(false);
    QPalette palette = m_errorLabel->palette();
    palette.setColor(QPalette::WindowText, creatorTheme()->color(Theme::TextColorError));
    m_errorLabel->setPalette(palette);
    vLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    vLayout->addWidget(m_errorLabel);
    setLayout(vLayout);
}

JsonFieldPage::~JsonFieldPage()
{
    // Do not delete m_expander, it belongs to the wizard!
    qDeleteAll(m_fields);
}

void JsonFieldPage::registerFieldFactory(const QString &id, const JsonFieldPage::FieldFactory &ff)
{
    QTC_ASSERT(!m_factories.contains(id), return);
    m_factories.insert(id, ff);
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
            if (f->isMandatory() && !f->widget()->isHidden())
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

JsonFieldPage::Field *JsonFieldPage::createFieldData(const QString &type)
{
    if (auto factory = m_factories.value(type)) {
        JsonFieldPage::Field *field = factory();
        field->setType(type);
        return field;
    }
    return nullptr;
}

} // namespace ProjectExplorer
