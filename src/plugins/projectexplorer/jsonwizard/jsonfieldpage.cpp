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

#include "../project.h"
#include "../projecttree.h"

#include <coreplugin/icore.h>
#include <coreplugin/locator/ilocatorfilter.h>
#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QCompleter>
#include <QDebug>
#include <QDir>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QItemSelectionModel>
#include <QLabel>
#include <QListView>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStandardItem>
#include <QTextEdit>
#include <QVariant>
#include <QVariantMap>
#include <QVBoxLayout>

using namespace Utils;

const char NAME_KEY[] = "name";
const char DISPLAY_NAME_KEY[] = "trDisplayName";
const char TOOLTIP_KEY[] = "trToolTip";
const char MANDATORY_KEY[] = "mandatory";
const char PERSISTENCE_KEY_KEY[] = "persistenceKey";
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
            typeAndName = QString("%1 (\"%2\")").arg(type, name);

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

    QValidator::State validate(QString &input, int &pos) const override
    {
        fixup(input);
        return QRegularExpressionValidator::validate(input, pos);
    }

    void fixup(QString &fixup) const override
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

JsonFieldPage::Field::Field() : d(std::make_unique<FieldPrivate>())
{ }

JsonFieldPage::Field::~Field()
{
    delete d->m_widget;
    delete d->m_label;
}

QString JsonFieldPage::Field::type()
{
    return d->m_type;
}

void JsonFieldPage::Field::setHasUserChanges()
{
    d->m_hasUserChanges = true;
}

void JsonFieldPage::Field::fromSettings(const QVariant &value)
{
    Q_UNUSED(value);
}

QVariant JsonFieldPage::Field::toSettings() const
{
    return {};
}

JsonFieldPage::Field *JsonFieldPage::Field::parse(const QVariant &input, QString *errorMessage)
{
    if (input.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field is not an object.");
        return nullptr;
    }

    QVariantMap tmp = input.toMap();
    const QString name = consumeValue(tmp, NAME_KEY).toString();
    if (name.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field has no name.");
        return nullptr;
    }
    const QString type = consumeValue(tmp, TYPE_KEY).toString();
    if (type.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field \"%1\" has no type.").arg(name);
        return nullptr;
    }

    Field *data = createFieldData(type);
    if (!data) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Field \"%1\" has unsupported type \"%2\".")
                .arg(name).arg(type);
        return nullptr;
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
    data->setPersistenceKey(consumeValue(tmp, PERSISTENCE_KEY_KEY).toString());

    QVariant dataVal = consumeValue(tmp, DATA_KEY);
    if (!data->parseData(dataVal, errorMessage)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "When parsing Field \"%1\": %2")
                .arg(name).arg(*errorMessage);
        delete data;
        return nullptr;
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

QString JsonFieldPage::Field::persistenceKey() const
{
    return d->m_persistenceKey;
}

bool JsonFieldPage::Field::isMandatory()
{
    return d->m_isMandatory;
}

bool JsonFieldPage::Field::hasSpan()
{
    return d->m_hasSpan;
}

bool JsonFieldPage::Field::hasUserChanges() const
{
    return d->m_hasUserChanges;
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

void JsonFieldPage::Field::setPersistenceKey(const QString &key)
{
    d->m_persistenceKey = key;
}

// --------------------------------------------------------------------
// LabelFieldData:
// --------------------------------------------------------------------

bool LabelField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Label (\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_wordWrap = consumeValue(tmp, "wordWrap", false).toBool();
    m_text = JsonWizardFactory::localizedString(consumeValue(tmp, "trText"));

    if (m_text.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Label (\"%1\") has no trText.")
                .arg(name());
        return false;
    }
    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}

QWidget *LabelField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName)
    Q_UNUSED(page)
    auto w = new QLabel;
    w->setWordWrap(m_wordWrap);
    w->setText(m_text);
    w->setSizePolicy(QSizePolicy::Expanding, w->sizePolicy().verticalPolicy());
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
                                                    "Spacer (\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    bool ok;
    m_factor = consumeValue(tmp, "factor", 1).toInt(&ok);

    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "Spacer (\"%1\") property \"factor\" is no integer value.")
                .arg(name());
        return false;
    }
    warnAboutUnsupportedKeys(tmp, name(), type());

    return true;
}

QWidget *SpacerField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName)
    Q_UNUSED(page)
    int hspace = QApplication::style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    int vspace = QApplication::style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    int hsize = hspace * m_factor;
    int vsize = vspace * m_factor;

    auto w = new QWidget();
    w->setMinimumSize(hsize, vsize);
    w->setMaximumSize(hsize, vsize);
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
                                                    "LineEdit (\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_isPassword = consumeValue(tmp, "isPassword", false).toBool();
    m_defaultText = JsonWizardFactory::localizedString(consumeValue(tmp, "trText").toString());
    m_disabledText = JsonWizardFactory::localizedString(consumeValue(tmp, "trDisabledText").toString());
    m_placeholderText = JsonWizardFactory::localizedString(consumeValue(tmp, "trPlaceholder").toString());
    m_historyId = consumeValue(tmp, "historyId").toString();
    m_restoreLastHistoryItem = consumeValue(tmp, "restoreLastHistoryItem", false).toBool();
    QString pattern = consumeValue(tmp, "validator").toString();
    if (!pattern.isEmpty()) {
        m_validatorRegExp = QRegularExpression(pattern);
        if (!m_validatorRegExp.isValid()) {
            *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                        "LineEdit (\"%1\") has an invalid regular expression \"%2\" in \"validator\".")
                    .arg(name(), pattern);
            m_validatorRegExp = QRegularExpression();
            return false;
        }
    }
    m_fixupExpando = consumeValue(tmp, "fixup").toString();

    const QString completion = consumeValue(tmp, "completion").toString();
    if (completion == "classes") {
        m_completion = Completion::Classes;
    } else if (completion == "namespaces") {
        m_completion = Completion::Namespaces;
    } else if (!completion.isEmpty()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                "LineEdit (\"%1\") has an invalid value \"%2\" in \"completion\".")
                .arg(name(), completion);
        return false;
    }

    warnAboutUnsupportedKeys(tmp, name(), type());

    return true;
}

QWidget *LineEditField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(displayName)
    auto w = new FancyLineEdit;

    if (m_validatorRegExp.isValid()) {
        auto lv = new LineEditValidator(page->expander(), m_validatorRegExp, w);
        lv->setFixupExpando(m_fixupExpando);
        w->setValidator(lv);
    }

    if (!m_historyId.isEmpty())
        w->setHistoryCompleter(m_historyId, m_restoreLastHistoryItem);

    w->setEchoMode(m_isPassword ? QLineEdit::Password : QLineEdit::Normal);
    QObject::connect(w, &FancyLineEdit::textEdited, [this] { setHasUserChanges(); });
    setupCompletion(w);

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

void LineEditField::fromSettings(const QVariant &value)
{
    m_defaultText = value.toString();
}

QVariant LineEditField::toSettings() const
{
    return qobject_cast<FancyLineEdit *>(widget())->text();
}

void LineEditField::setupCompletion(FancyLineEdit *lineEdit)
{
    using namespace Core;
    using namespace Utils;
    if (m_completion == Completion::None)
        return;
    ILocatorFilter * const classesFilter = findOrDefault(
                ILocatorFilter::allLocatorFilters(),
                equal(&ILocatorFilter::id, Id("Classes")));
    if (!classesFilter)
        return;
    classesFilter->prepareSearch({});
    const auto watcher = new QFutureWatcher<LocatorFilterEntry>(lineEdit);
    const auto handleResults = [this, lineEdit, watcher](int firstIndex, int endIndex) {
        QSet<QString> namespaces;
        QStringList classes;
        QString projectBaseDir;
        Project * const project = ProjectTree::currentProject();
        for (int i = firstIndex; i < endIndex; ++i) {
            static const auto isReservedName = [](const QString &name) {
                static const QRegularExpression rx1("^_[A-Z].*");
                static const QRegularExpression rx2(".*::_[A-Z].*");
                return name.contains("__") || rx1.match(name).hasMatch()
                        || rx2.match(name).hasMatch();
            };
            const LocatorFilterEntry &entry = watcher->resultAt(i);
            const bool hasNamespace = !entry.extraInfo.isEmpty()
                    && !entry.extraInfo.startsWith('<')  && !entry.extraInfo.contains("::<")
                    && !isReservedName(entry.extraInfo)
                    && !entry.extraInfo.startsWith('~')
                    && !entry.extraInfo.contains("Anonymous:")
                    && !FileUtils::isAbsolutePath(entry.extraInfo);
            const bool isBaseClassCandidate = !isReservedName(entry.displayName)
                    && !entry.displayName.startsWith("Anonymous:");
            if (isBaseClassCandidate)
                classes << entry.displayName;
            if (hasNamespace) {
                if (isBaseClassCandidate)
                    classes << (entry.extraInfo + "::" + entry.displayName);
                if (m_completion == Completion::Namespaces) {
                    if (!project
                            || entry.fileName.startsWith(project->projectDirectory().toString())) {
                        namespaces << entry.extraInfo;
                    }
                }
            }
        }
        QStringList completionList;
        if (m_completion == Completion::Namespaces) {
            completionList = toList(namespaces);
            completionList = filtered(completionList, [&classes](const QString &ns) {
                return !classes.contains(ns);
            });
            completionList = transform(completionList, [](const QString &ns) {
                return QString(ns + "::");
            });
        } else {
            completionList = classes;
        }
        completionList.sort();
        lineEdit->setSpecialCompleter(new QCompleter(completionList, lineEdit));
        watcher->deleteLater();
    };
    QObject::connect(watcher, &QFutureWatcher<LocatorFilterEntry>::resultsReadyAt, lineEdit,
                     handleResults);
    watcher->setFuture(runAsync([classesFilter](QFutureInterface<LocatorFilterEntry> &f) {
        const QList<LocatorFilterEntry> matches = classesFilter->matchesFor(f, {});
        f.reportResults(QVector<LocatorFilterEntry>(matches.cbegin(), matches.cend()));
        f.reportFinished();
    }));
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
                                                    "TextEdit (\"%1\") data is not an object.")
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
    Q_UNUSED(displayName)
    Q_UNUSED(page)
    auto w = new QTextEdit;
    w->setAcceptRichText(m_acceptRichText);
    QObject::connect(w, &QTextEdit::textChanged, [this, w] {
       if (w->toPlainText() != m_defaultText)
           setHasUserChanges();
    });
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

void TextEditField::fromSettings(const QVariant &value)
{
    m_defaultText = value.toString();
}

QVariant TextEditField::toSettings() const
{
    return qobject_cast<QTextEdit *>(widget())->toPlainText();
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
    Q_UNUSED(displayName)
    Q_UNUSED(page)
    auto w = new PathChooser;
    if (!m_historyId.isEmpty())
        w->setHistoryCompleter(m_historyId);
    QObject::connect(w, &PathChooser::pathChanged, [this, w] {
        if (w->filePath().toString() != m_path)
            setHasUserChanges();
    });
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
                     page, [page](QString) { emit page->completeChanged(); });
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
    w->setBaseDirectory(expander->expand(FilePath::fromString(m_basePath)));
    w->setExpectedKind(m_kind);

    if (m_currentPath.isNull())
        w->setPath(expander->expand(m_path));
    else
        w->setPath(m_currentPath);
}

void PathChooserField::fromSettings(const QVariant &value)
{
    m_path = value.toString();
}

QVariant PathChooserField::toSettings() const
{
    return qobject_cast<PathChooser *>(widget())->filePath().toString();
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
                                                    "CheckBox (\"%1\") data is not an object.")
                .arg(name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    m_checkedValue = consumeValue(tmp, "checkedValue", true).toString();
    m_uncheckedValue = consumeValue(tmp, "uncheckedValue", false).toString();
    if (m_checkedValue == m_uncheckedValue) {
        *errorMessage= QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                   "CheckBox (\"%1\") values for checked and unchecked state are identical.")
                .arg(name());
       return false;
    }
    m_checkedExpression = consumeValue(tmp, "checked", false);

    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}

QWidget *CheckBoxField::createWidget(const QString &displayName, JsonFieldPage *page)
{
    Q_UNUSED(page)
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

    QObject::connect(w, &QCheckBox::clicked, page, [this, page]() {
        m_isModified = true;
        setHasUserChanges();
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

void CheckBoxField::fromSettings(const QVariant &value)
{
    m_checkedExpression = value;
}

QVariant CheckBoxField::toSettings() const
{
    return qobject_cast<QCheckBox *>(widget())->isChecked();
}

// --------------------------------------------------------------------
// ListFieldData:
// --------------------------------------------------------------------

std::unique_ptr<QStandardItem> createStandardItemFromListItem(const QVariant &item, QString *errorMessage)
{
    if (item.type() == QVariant::List) {
        *errorMessage  = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                     "No JSON lists allowed inside List items.");
        return {};
    }
    auto standardItem = std::make_unique<QStandardItem>();
    if (item.type() == QVariant::Map) {
        QVariantMap tmp = item.toMap();
        const QString key = JsonWizardFactory::localizedString(consumeValue(tmp, "trKey", QString()).toString());
        const QVariant value = consumeValue(tmp, "value", key);

        if (key.isNull() || key.isEmpty()) {
            *errorMessage  = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                         "No \"key\" found in List items.");
            return {};
        }
        standardItem->setText(key);
        standardItem->setData(value, ListField::ValueRole);
        standardItem->setData(consumeValue(tmp, "condition", true), ListField::ConditionRole);
        standardItem->setData(consumeValue(tmp, "icon"), ListField::IconStringRole);
        standardItem->setToolTip(JsonWizardFactory::localizedString(consumeValue(tmp, "trToolTip", QString()).toString()));
        warnAboutUnsupportedKeys(tmp, QString(), "List");
    } else {
        const QString keyvalue = item.toString();
        standardItem->setText(keyvalue);
        standardItem->setData(keyvalue, ListField::ValueRole);
        standardItem->setData(true, ListField::ConditionRole);
    }
    return standardItem;
}

ListField::ListField() = default;

ListField::~ListField() = default;

bool ListField::parseData(const QVariant &data, QString *errorMessage)
{
    if (data.type() != QVariant::Map) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "%1 (\"%2\") data is not an object.")
                .arg(type(), name());
        return false;
    }

    QVariantMap tmp = data.toMap();

    bool ok;
    m_index = consumeValue(tmp, "index", 0).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "%1 (\"%2\") \"index\" is not an integer value.")
                .arg(type(), name());
        return false;
    }
    m_disabledIndex = consumeValue(tmp, "disabledIndex", -1).toInt(&ok);
    if (!ok) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "%1 (\"%2\") \"disabledIndex\" is not an integer value.")
                .arg(type(), name());
        return false;
    }

    const QVariant value = consumeValue(tmp, "items");
    if (value.isNull()) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "%1 (\"%2\") \"items\" missing.")
                .arg(type(), name());
        return false;
    }
    if (value.type() != QVariant::List) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonFieldPage",
                                                    "%1 (\"%2\") \"items\" is not a JSON list.")
                .arg(type(), name());
        return false;
    }

    for (const QVariant &i : value.toList()) {
        std::unique_ptr<QStandardItem> item = createStandardItemFromListItem(i, errorMessage);
        QTC_ASSERT(!item || !item->text().isEmpty(), continue);
        m_itemList.emplace_back(std::move(item));
    }

    warnAboutUnsupportedKeys(tmp, name(), type());
    return true;
}


bool ListField::validate(MacroExpander *expander, QString *message)
{
    if (!JsonFieldPage::Field::validate(expander, message))
        return false;

    updateIndex();
    return selectionModel()->hasSelection();
}

void ListField::initializeData(MacroExpander *expander)
{
    QTC_ASSERT(widget(), return);

    if (m_index >= int(m_itemList.size())) {
        qWarning().noquote() <<  QString("%1 (\"%2\") has an index of %3 which does not exist.").arg(type(), name(), QString::number(m_index));
        m_index = -1;
    }

    QStandardItem *currentItem = m_index >= 0 ? m_itemList[uint(m_index)].get() : nullptr;
    QList<QStandardItem*> expandedValuesItems;
    expandedValuesItems.reserve(int(m_itemList.size()));

    for (const std::unique_ptr<QStandardItem> &item : m_itemList) {
        bool condition = JsonWizard::boolFromVariant(item->data(ConditionRole), expander);
        if (!condition)
            continue;
        QStandardItem *expandedValuesItem = item->clone();
        if (item.get() == currentItem)
            currentItem = expandedValuesItem;
        expandedValuesItem->setText(expander->expand(item->text()));
        expandedValuesItem->setData(expander->expandVariant(item->data(ValueRole)), ValueRole);
        expandedValuesItem->setData(expander->expand(item->data(IconStringRole).toString()), IconStringRole);
        expandedValuesItem->setData(condition, ConditionRole);

        QString iconPath = expandedValuesItem->data(IconStringRole).toString();
        if (!iconPath.isEmpty()) {
            if (auto *page = qobject_cast<JsonFieldPage*>(widget()->parentWidget())) {
                const QString wizardDirectory = page->value("WizardDir").toString();
                iconPath = QDir::cleanPath(QDir(wizardDirectory).absoluteFilePath(iconPath));
                if (QFileInfo::exists(iconPath)) {
                    QIcon icon(iconPath);
                    expandedValuesItem->setIcon(icon);
                    addPossibleIconSize(icon);
                } else {
                    qWarning().noquote() << QString("Icon file \"%1\" not found.").arg(QDir::toNativeSeparators(iconPath));
                }
            } else {
                qWarning().noquote() <<  QString("%1 (\"%2\") has no parentWidget JsonFieldPage to get the icon path.").arg(type(), name());
            }
        }
        expandedValuesItems.append(expandedValuesItem);
    }

    itemModel()->clear();
    itemModel()->appendColumn(expandedValuesItems); // inserts the first column

    selectionModel()->setCurrentIndex(itemModel()->indexFromItem(currentItem), QItemSelectionModel::ClearAndSelect);

    updateIndex();
}

QStandardItemModel *ListField::itemModel()
{
    if (!m_itemModel)
        m_itemModel = new QStandardItemModel(widget());
    return m_itemModel;
}

QItemSelectionModel *ListField::selectionModel() const
{
    return m_selectionModel;
}

void ListField::setSelectionModel(QItemSelectionModel *selectionModel)
{
    m_selectionModel = selectionModel;
}

QSize ListField::maxIconSize()
{
    return m_maxIconSize;
}

void ListField::addPossibleIconSize(const QIcon &icon)
{
    const QSize iconSize = icon.availableSizes().value(0);
    if (iconSize.height() > m_maxIconSize.height())
        m_maxIconSize = iconSize;
}

void ListField::updateIndex()
{
    if (!widget()->isEnabled() && m_disabledIndex >= 0 && m_savedIndex < 0) {
        m_savedIndex = selectionModel()->currentIndex().row();
        selectionModel()->setCurrentIndex(itemModel()->index(m_disabledIndex, 0), QItemSelectionModel::ClearAndSelect);
    } else if (widget()->isEnabled() && m_savedIndex >= 0) {
        selectionModel()->setCurrentIndex(itemModel()->index(m_savedIndex, 0), QItemSelectionModel::ClearAndSelect);
        m_savedIndex = -1;
    }
}

void ListField::fromSettings(const QVariant &value)
{
    for (decltype(m_itemList)::size_type i = 0; i < m_itemList.size(); ++i) {
        if (m_itemList.at(i)->data(ValueRole) == value) {
            m_index = int(i);
            break;
        }
    }
}

QVariant ListField::toSettings() const
{
    const int idx = selectionModel()->currentIndex().row();
    return idx >= 0 ? m_itemList.at(idx)->data(ValueRole) : QVariant();
}

void ComboBoxField::setup(JsonFieldPage *page, const QString &name)
{
    auto w = qobject_cast<QComboBox*>(widget());
    QTC_ASSERT(w, return);
    w->setModel(itemModel());
    w->setInsertPolicy(QComboBox::NoInsert);

    QSizePolicy s = w->sizePolicy();
    s.setHorizontalPolicy(QSizePolicy::Expanding);
    w->setSizePolicy(s);

    setSelectionModel(w->view()->selectionModel());

    // the selectionModel does not behave like expected and wanted - so we block signals here
    // (for example there was some losing focus thing when hovering over items, ...)
    selectionModel()->blockSignals(true);
    QObject::connect(w, QOverload<int>::of(&QComboBox::activated), [w, this](int index) {
        w->blockSignals(true);
        selectionModel()->clearSelection();

        selectionModel()->blockSignals(false);
        selectionModel()->setCurrentIndex(w->model()->index(index, 0),
            QItemSelectionModel::ClearAndSelect);
        selectionModel()->blockSignals(true);
        w->blockSignals(false);
    });
    page->registerObjectAsFieldWithName<QComboBox>(name, w, QOverload<int>::of(&QComboBox::activated), [w]() {
        return w->currentData(ValueRole);
    });
    QObject::connect(selectionModel(), &QItemSelectionModel::selectionChanged, page, [page]() {
        emit page->completeChanged();
    });
}

QWidget *ComboBoxField::createWidget(const QString & /*displayName*/, JsonFieldPage * /*page*/)
{
    const auto comboBox = new QComboBox;
    QObject::connect(comboBox, QOverload<int>::of(&QComboBox::activated),
                     [this] { setHasUserChanges(); });
    return comboBox;
}

void ComboBoxField::initializeData(MacroExpander *expander)
{
    ListField::initializeData(expander);
    // refresh also the current text of the combobox
    auto w = qobject_cast<QComboBox*>(widget());
    w->setCurrentIndex(selectionModel()->currentIndex().row());
}

QVariant ComboBoxField::toSettings() const
{
    if (auto w = qobject_cast<QComboBox*>(widget()))
        return w->currentData(ValueRole);
    return {};
}

void IconListField::setup(JsonFieldPage *page, const QString &name)
{
    auto w = qobject_cast<QListView*>(widget());
    QTC_ASSERT(w, return);

    w->setViewMode(QListView::IconMode);
    w->setMovement(QListView::Static);
    w->setResizeMode(QListView::Adjust);
    w->setSelectionRectVisible(false);
    w->setWrapping(true);
    w->setWordWrap(true);

    w->setModel(itemModel());
    setSelectionModel(w->selectionModel());
    page->registerObjectAsFieldWithName<QItemSelectionModel>(name, selectionModel(), &QItemSelectionModel::selectionChanged, [this]() {
        const QModelIndex i = selectionModel()->currentIndex();
        if (i.isValid())
            return i.data(ValueRole);
        return QVariant();
    });
    QObject::connect(selectionModel(), &QItemSelectionModel::selectionChanged, page, [page]() {
        emit page->completeChanged();
    });
}

QWidget *IconListField::createWidget(const QString & /*displayName*/, JsonFieldPage * /*page*/)
{
    const auto listView = new QListView;
    QObject::connect(listView->selectionModel(), &QItemSelectionModel::currentChanged,
                     [this] { setHasUserChanges(); });
    return listView;
}

void IconListField::initializeData(MacroExpander *expander)
{
    ListField::initializeData(expander);
    auto w = qobject_cast<QListView*>(widget());
    const int spacing = 4;
    w->setSpacing(spacing);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // adding a third hight of the icon to see following items if there are some
    w->setMinimumHeight(maxIconSize().height() + maxIconSize().height() / 3);
    w->setIconSize(maxIconSize());
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
        if (!f->persistenceKey().isEmpty()) {
            f->setPersistenceKey(m_expander->expand(f->persistenceKey()));
            const QVariant value = Core::ICore::settings()
                    ->value(fullSettingsKey(f->persistenceKey()));
            if (value.isValid())
                f->fromSettings(value);
        }
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

bool JsonFieldPage::validatePage()
{
    for (Field * const f : qAsConst(m_fields))
        if (!f->persistenceKey().isEmpty() && f->hasUserChanges()) {
            const QVariant value = f->toSettings();
            if (value.isValid())
                Core::ICore::settings()->setValue(fullSettingsKey(f->persistenceKey()), value);
    }
    return true;
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

QString JsonFieldPage::fullSettingsKey(const QString &fieldKey)
{
    return "Wizards/" + fieldKey;
}

} // namespace ProjectExplorer
