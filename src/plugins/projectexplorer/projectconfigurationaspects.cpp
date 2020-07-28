/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "projectconfigurationaspects.h"

#include "environmentaspect.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runconfiguration.h"
#include "target.h"

#include <coreplugin/variablechooser.h>
#include <utils/utilsicons.h>
#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QSpinBox>
#include <QToolButton>
#include <QTextEdit>
#include <QRadioButton>
#include <QButtonGroup>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class BaseBoolAspectPrivate
{
public:
    BaseBoolAspect::LabelPlacement m_labelPlacement = BaseBoolAspect::LabelPlacement::AtCheckBox;
    bool m_value = false;
    bool m_defaultValue = false;
    bool m_enabled = true;
    QString m_label;
    QString m_tooltip;
    QPointer<QCheckBox> m_checkBox; // Owned by configuration widget
};

class BaseSelectionAspectPrivate
{
public:
    int m_value = 0;
    int m_defaultValue = 0;
    BaseSelectionAspect::DisplayStyle m_displayStyle
            = BaseSelectionAspect::DisplayStyle::RadioButtons;
    struct Option { QString displayName; QString tooltip; };
    QVector<Option> m_options;

    // These are all owned by the configuration widget.
    QList<QPointer<QRadioButton>> m_buttons;
    QPointer<QComboBox> m_comboBox;
    QPointer<QLabel> m_label;
    QPointer<QButtonGroup> m_buttonGroup;
};

class BaseStringAspectPrivate
{
public:
    BaseStringAspect::DisplayStyle m_displayStyle = BaseStringAspect::LabelDisplay;
    BaseStringAspect::CheckBoxPlacement m_checkBoxPlacement
        = BaseStringAspect::CheckBoxPlacement::Right;
    BaseStringAspect::UncheckedSemantics m_uncheckedSemantics
        = BaseStringAspect::UncheckedSemantics::Disabled;
    QString m_labelText;
    std::function<QString(const QString &)> m_displayFilter;
    std::unique_ptr<BaseBoolAspect> m_checker;

    QString m_value;
    QString m_placeHolderText;
    QString m_historyCompleterKey;
    PathChooser::Kind m_expectedKind = PathChooser::File;
    Environment m_environment;
    QPointer<QLabel> m_label;
    QPointer<QLabel> m_labelDisplay;
    QPointer<FancyLineEdit> m_lineEditDisplay;
    QPointer<PathChooser> m_pathChooserDisplay;
    QPointer<QTextEdit> m_textEditDisplay;
    Utils::MacroExpanderProvider m_expanderProvider;
    QPixmap m_labelPixmap;
    Utils::FilePath m_baseFileName;
    BaseStringAspect::ValueAcceptor m_valueAcceptor;
    bool m_readOnly = false;
    bool m_showToolTipOnLabel = false;
    bool m_fileDialogOnly = false;

    template<class Widget> void updateWidgetFromCheckStatus(Widget *w)
    {
        const bool enabled = !m_checker || m_checker->value();
        if (m_uncheckedSemantics == BaseStringAspect::UncheckedSemantics::Disabled)
            w->setEnabled(enabled);
        else
            w->setReadOnly(!enabled);
    }
};

class BaseIntegerAspectPrivate
{
public:
    qint64 m_value = 0;
    qint64 m_defaultValue = 0;
    QVariant m_minimumValue;
    QVariant m_maximumValue;
    int m_displayIntegerBase = 10;
    qint64 m_displayScaleFactor = 1;
    QString m_labelText;
    QString m_prefix;
    QString m_suffix;
    QString m_tooltip;
    QPointer<QLabel> m_label;
    QPointer<QSpinBox> m_spinBox; // Owned by configuration widget
    bool m_enabled = true;
};

class BaseStringListAspectPrivate
{
public:
    QStringList m_value;
};

} // Internal

/*!
    \class ProjectExplorer::BaseStringAspect
*/

BaseStringAspect::BaseStringAspect()
    : d(new Internal::BaseStringAspectPrivate)
{}

BaseStringAspect::~BaseStringAspect() = default;

void BaseStringAspect::setValueAcceptor(BaseStringAspect::ValueAcceptor &&acceptor)
{
    d->m_valueAcceptor = std::move(acceptor);
}

QString BaseStringAspect::value() const
{
    return d->m_value;
}

void BaseStringAspect::setValue(const QString &value)
{
    const bool isSame = value == d->m_value;
    if (isSame)
        return;

    QString processedValue = value;
    if (d->m_valueAcceptor) {
        const Utils::optional<QString> tmp = d->m_valueAcceptor(d->m_value, value);
        if (!tmp) {
            update(); // Make sure the original value is retained in the UI
            return;
        }
        processedValue = tmp.value();
    }

    d->m_value = processedValue;
    update();
    emit changed();
}

void BaseStringAspect::fromMap(const QVariantMap &map)
{
    if (!settingsKey().isEmpty())
        d->m_value = map.value(settingsKey()).toString();
    if (d->m_checker)
        d->m_checker->fromMap(map);
}

void BaseStringAspect::toMap(QVariantMap &map) const
{
    if (!settingsKey().isEmpty())
        map.insert(settingsKey(), d->m_value);
    if (d->m_checker)
        d->m_checker->toMap(map);
}

FilePath BaseStringAspect::filePath() const
{
    return FilePath::fromUserInput(d->m_value);
}

void BaseStringAspect::setFilePath(const FilePath &val)
{
    setValue(val.toUserOutput());
}

void BaseStringAspect::setLabelText(const QString &labelText)
{
    d->m_labelText = labelText;
    if (d->m_label)
        d->m_label->setText(labelText);
}

void BaseStringAspect::setLabelPixmap(const QPixmap &labelPixmap)
{
    d->m_labelPixmap = labelPixmap;
    if (d->m_label)
        d->m_label->setPixmap(labelPixmap);
}

void BaseStringAspect::setShowToolTipOnLabel(bool show)
{
    d->m_showToolTipOnLabel = show;
    update();
}

QString BaseStringAspect::labelText() const
{
    return d->m_labelText;
}

void BaseStringAspect::setDisplayFilter(const std::function<QString(const QString &)> &displayFilter)
{
    d->m_displayFilter = displayFilter;
}

bool BaseStringAspect::isChecked() const
{
    return !d->m_checker || d->m_checker->value();
}

void BaseStringAspect::setChecked(bool checked)
{
    QTC_ASSERT(d->m_checker, return);
    d->m_checker->setValue(checked);
}

void BaseStringAspect::setDisplayStyle(DisplayStyle displayStyle)
{
    d->m_displayStyle = displayStyle;
}

void BaseStringAspect::setPlaceHolderText(const QString &placeHolderText)
{
    d->m_placeHolderText = placeHolderText;
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->setPlaceholderText(placeHolderText);
    if (d->m_textEditDisplay)
        d->m_textEditDisplay->setPlaceholderText(placeHolderText);
}

void BaseStringAspect::setHistoryCompleter(const QString &historyCompleterKey)
{
    d->m_historyCompleterKey = historyCompleterKey;
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->setHistoryCompleter(historyCompleterKey);
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setHistoryCompleter(historyCompleterKey);
}

void BaseStringAspect::setExpectedKind(const PathChooser::Kind expectedKind)
{
    d->m_expectedKind = expectedKind;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setExpectedKind(expectedKind);
}

void BaseStringAspect::setFileDialogOnly(bool requireFileDialog)
{
    d->m_fileDialogOnly = requireFileDialog;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setFileDialogOnly(requireFileDialog);
}

void BaseStringAspect::setEnvironment(const Environment &env)
{
    d->m_environment = env;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setEnvironment(env);
}

void BaseStringAspect::setBaseFileName(const FilePath &baseFileName)
{
    d->m_baseFileName = baseFileName;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setBaseDirectory(baseFileName);
}

void BaseStringAspect::setReadOnly(bool readOnly)
{
    d->m_readOnly = readOnly;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setReadOnly(readOnly);
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->setReadOnly(readOnly);
    if (d->m_textEditDisplay)
        d->m_textEditDisplay->setReadOnly(readOnly);
}

void BaseStringAspect::setMacroExpanderProvider(const MacroExpanderProvider &expanderProvider)
{
    d->m_expanderProvider = expanderProvider;
}

void BaseStringAspect::validateInput()
{
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->triggerChanged();
    if (d->m_lineEditDisplay)
        d->m_lineEditDisplay->validate();
}

void BaseStringAspect::setUncheckedSemantics(BaseStringAspect::UncheckedSemantics semantics)
{
    d->m_uncheckedSemantics = semantics;
}

void BaseStringAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!d->m_label);

    if (d->m_checker && d->m_checkBoxPlacement == CheckBoxPlacement::Top) {
        d->m_checker->addToLayout(builder);
        builder.startNewRow();
    }

    d->m_label = new QLabel;
    d->m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    d->m_label->setText(d->m_labelText);
    if (!d->m_labelPixmap.isNull())
        d->m_label->setPixmap(d->m_labelPixmap);
    builder.addItem(d->m_label.data());

    const auto useMacroExpander = [this, &builder](QWidget *w) {
        if (!d->m_expanderProvider)
            return;
        const auto chooser = new Core::VariableChooser(builder.layout()->parentWidget());
        chooser->addSupportedWidget(w);
        chooser->addMacroExpanderProvider(d->m_expanderProvider);
    };

    switch (d->m_displayStyle) {
    case PathChooserDisplay:
        d->m_pathChooserDisplay = new PathChooser;
        d->m_pathChooserDisplay->setExpectedKind(d->m_expectedKind);
        if (!d->m_historyCompleterKey.isEmpty())
            d->m_pathChooserDisplay->setHistoryCompleter(d->m_historyCompleterKey);
        d->m_pathChooserDisplay->setEnvironment(d->m_environment);
        d->m_pathChooserDisplay->setBaseDirectory(d->m_baseFileName);
        d->m_pathChooserDisplay->setReadOnly(d->m_readOnly);
        useMacroExpander(d->m_pathChooserDisplay->lineEdit());
        connect(d->m_pathChooserDisplay, &PathChooser::pathChanged,
                this, &BaseStringAspect::setValue);
        builder.addItem(d->m_pathChooserDisplay.data());
        d->m_pathChooserDisplay->setFileDialogOnly(d->m_fileDialogOnly);
        break;
    case LineEditDisplay:
        d->m_lineEditDisplay = new FancyLineEdit;
        d->m_lineEditDisplay->setPlaceholderText(d->m_placeHolderText);
        if (!d->m_historyCompleterKey.isEmpty())
            d->m_lineEditDisplay->setHistoryCompleter(d->m_historyCompleterKey);
        d->m_lineEditDisplay->setReadOnly(d->m_readOnly);
        useMacroExpander(d->m_lineEditDisplay);
        connect(d->m_lineEditDisplay, &FancyLineEdit::textEdited,
                this, &BaseStringAspect::setValue);
        builder.addItem(d->m_lineEditDisplay.data());
        break;
    case TextEditDisplay:
        d->m_textEditDisplay = new QTextEdit;
        d->m_textEditDisplay->setPlaceholderText(d->m_placeHolderText);
        d->m_textEditDisplay->setReadOnly(d->m_readOnly);
        useMacroExpander(d->m_textEditDisplay);
        connect(d->m_textEditDisplay, &QTextEdit::textChanged, this, [this] {
            const QString value = d->m_textEditDisplay->document()->toPlainText();
            if (value != d->m_value) {
                d->m_value = value;
                emit changed();
            }
        });
        builder.addItem(d->m_textEditDisplay.data());
        break;
    case LabelDisplay:
        d->m_labelDisplay = new QLabel;
        d->m_labelDisplay->setTextInteractionFlags(Qt::TextSelectableByMouse);
        builder.addItem(d->m_labelDisplay.data());
        break;
    }

    validateInput();

    if (d->m_checker && d->m_checkBoxPlacement == CheckBoxPlacement::Right)
        d->m_checker->addToLayout(builder);

    update();
}

void BaseStringAspect::update()
{
    const QString displayedString = d->m_displayFilter ? d->m_displayFilter(d->m_value)
                                                       : d->m_value;

    if (d->m_pathChooserDisplay) {
        d->m_pathChooserDisplay->setFilePath(FilePath::fromString(displayedString));
        d->updateWidgetFromCheckStatus(d->m_pathChooserDisplay.data());
    }

    if (d->m_lineEditDisplay) {
        d->m_lineEditDisplay->setTextKeepingActiveCursor(displayedString);
        d->updateWidgetFromCheckStatus(d->m_lineEditDisplay.data());
    }

    if (d->m_textEditDisplay) {
        d->m_textEditDisplay->setText(displayedString);
        d->updateWidgetFromCheckStatus(d->m_textEditDisplay.data());
    }

    if (d->m_labelDisplay) {
        d->m_labelDisplay->setText(displayedString);
        d->m_labelDisplay->setToolTip(d->m_showToolTipOnLabel ? displayedString : QString());
    }

    if (d->m_label) {
        d->m_label->setText(d->m_labelText);
        if (!d->m_labelPixmap.isNull())
            d->m_label->setPixmap(d->m_labelPixmap);
    }

    validateInput();
}

void BaseStringAspect::makeCheckable(CheckBoxPlacement checkBoxPlacement,
                                     const QString &checkerLabel, const QString &checkerKey)
{
    QTC_ASSERT(!d->m_checker, return);
    d->m_checkBoxPlacement = checkBoxPlacement;
    d->m_checker.reset(new BaseBoolAspect);
    d->m_checker->setLabel(checkerLabel, checkBoxPlacement == CheckBoxPlacement::Top
                           ? BaseBoolAspect::LabelPlacement::InExtraLabel
                           : BaseBoolAspect::LabelPlacement::AtCheckBox);
    d->m_checker->setSettingsKey(checkerKey);

    connect(d->m_checker.get(), &BaseBoolAspect::changed, this, &BaseStringAspect::update);
    connect(d->m_checker.get(), &BaseBoolAspect::changed, this, &BaseStringAspect::changed);
    connect(d->m_checker.get(), &BaseBoolAspect::changed, this, &BaseStringAspect::checkedChanged);

    update();
}

/*!
    \class ProjectExplorer::BaseBoolAspect
*/

BaseBoolAspect::BaseBoolAspect(const QString &settingsKey)
    : d(new Internal::BaseBoolAspectPrivate)
{
    setSettingsKey(settingsKey);
}

BaseBoolAspect::~BaseBoolAspect() = default;

void BaseBoolAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!d->m_checkBox);
    d->m_checkBox = new QCheckBox();
    if (d->m_labelPlacement == LabelPlacement::AtCheckBox) {
        d->m_checkBox->setText(d->m_label);
        builder.addItem(new QLabel);
    } else {
        builder.addItem(d->m_label);
    }
    d->m_checkBox->setChecked(d->m_value);
    d->m_checkBox->setToolTip(d->m_tooltip);
    d->m_checkBox->setEnabled(d->m_enabled);
    builder.addItem(d->m_checkBox.data());
    connect(d->m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        d->m_value = d->m_checkBox->isChecked();
        emit changed();
    });
}

void BaseBoolAspect::fromMap(const QVariantMap &map)
{
    d->m_value = map.value(settingsKey(), d->m_defaultValue).toBool();
}

void BaseBoolAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), d->m_value);
}

bool BaseBoolAspect::defaultValue() const
{
    return d->m_defaultValue;
}

void BaseBoolAspect::setDefaultValue(bool defaultValue)
{
    d->m_defaultValue = defaultValue;
    d->m_value = defaultValue;
}

bool BaseBoolAspect::value() const
{
    return d->m_value;
}

void BaseBoolAspect::setValue(bool value)
{
    d->m_value = value;
    if (d->m_checkBox)
        d->m_checkBox->setChecked(d->m_value);
}

void BaseBoolAspect::setLabel(const QString &label, LabelPlacement labelPlacement)
{
    d->m_label = label;
    d->m_labelPlacement = labelPlacement;
}

void BaseBoolAspect::setToolTip(const QString &tooltip)
{
    d->m_tooltip = tooltip;
}

void BaseBoolAspect::setEnabled(bool enabled)
{
    d->m_enabled = enabled;
    if (d->m_checkBox)
        d->m_checkBox->setEnabled(enabled);
}

/*!
    \class ProjectExplorer::BaseSelectionAspect
*/

BaseSelectionAspect::BaseSelectionAspect()
    : d(new Internal::BaseSelectionAspectPrivate)
{}

BaseSelectionAspect::~BaseSelectionAspect() = default;

void BaseSelectionAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(d->m_buttonGroup == nullptr);
    QTC_CHECK(!d->m_comboBox);
    QTC_ASSERT(d->m_buttons.isEmpty(), d->m_buttons.clear());

    switch (d->m_displayStyle) {
    case DisplayStyle::RadioButtons:
        d->m_buttonGroup = new QButtonGroup;
        d->m_buttonGroup->setExclusive(true);
        for (int i = 0, n = d->m_options.size(); i < n; ++i) {
            const Internal::BaseSelectionAspectPrivate::Option &option = d->m_options.at(i);
            auto button = new QRadioButton(option.displayName);
            button->setChecked(i == d->m_value);
            button->setToolTip(option.tooltip);
            builder.addItems(QString(), button);
            d->m_buttons.append(button);
            d->m_buttonGroup->addButton(button);
            connect(button, &QAbstractButton::clicked, this, [this, i] {
                d->m_value = i;
                emit changed();
            });
        }
        break;
    case DisplayStyle::ComboBox:
        d->m_label = new QLabel(displayName());
        d->m_comboBox = new QComboBox;
        for (int i = 0, n = d->m_options.size(); i < n; ++i)
            d->m_comboBox->addItem(d->m_options.at(i).displayName);
        connect(d->m_comboBox.data(), QOverload<int>::of(&QComboBox::activated), this,
                [this](int index) { d->m_value = index; emit changed(); });
        d->m_comboBox->setCurrentIndex(d->m_value);
        builder.addItems(d->m_label.data(), d->m_comboBox.data());
        break;
    }
}

void BaseSelectionAspect::fromMap(const QVariantMap &map)
{
    d->m_value = map.value(settingsKey(), d->m_defaultValue).toInt();
}

void BaseSelectionAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), d->m_value);
}

void BaseSelectionAspect::setVisibleDynamic(bool visible)
{
    if (d->m_label)
        d->m_label->setVisible(visible);
    if (d->m_comboBox)
        d->m_comboBox->setVisible(visible);
    for (QRadioButton * const button : qAsConst(d->m_buttons))
        button->setVisible(visible);
}

int BaseSelectionAspect::defaultValue() const
{
    return d->m_defaultValue;
}

void BaseSelectionAspect::setDefaultValue(int defaultValue)
{
    d->m_defaultValue = defaultValue;
}

void BaseSelectionAspect::setDisplayStyle(BaseSelectionAspect::DisplayStyle style)
{
    d->m_displayStyle = style;
}

int BaseSelectionAspect::value() const
{
    return d->m_value;
}

void BaseSelectionAspect::setValue(int value)
{
    d->m_value = value;
    if (d->m_buttonGroup && 0 <= value && value < d->m_buttons.size())
        d->m_buttons.at(value)->setChecked(true);
    else if (d->m_comboBox) {
        d->m_comboBox->setCurrentIndex(value);
    }
}

void BaseSelectionAspect::addOption(const QString &displayName, const QString &toolTip)
{
    d->m_options.append({displayName, toolTip});
}

/*!
    \class ProjectExplorer::BaseIntegerAspect
*/

// BaseIntegerAspect

BaseIntegerAspect::BaseIntegerAspect()
    : d(new Internal::BaseIntegerAspectPrivate)
{}

BaseIntegerAspect::~BaseIntegerAspect() = default;

void BaseIntegerAspect::addToLayout(LayoutBuilder &builder)
{
    QTC_CHECK(!d->m_label);
    d->m_label = new QLabel(d->m_labelText);
    d->m_label->setEnabled(d->m_enabled);

    QTC_CHECK(!d->m_spinBox);
    d->m_spinBox = new QSpinBox;
    d->m_spinBox->setValue(int(d->m_value / d->m_displayScaleFactor));
    d->m_spinBox->setDisplayIntegerBase(d->m_displayIntegerBase);
    d->m_spinBox->setPrefix(d->m_prefix);
    d->m_spinBox->setSuffix(d->m_suffix);
    d->m_spinBox->setEnabled(d->m_enabled);
    d->m_spinBox->setToolTip(d->m_tooltip);
    if (d->m_maximumValue.isValid() && d->m_maximumValue.isValid())
        d->m_spinBox->setRange(int(d->m_minimumValue.toLongLong() / d->m_displayScaleFactor),
                               int(d->m_maximumValue.toLongLong() / d->m_displayScaleFactor));

    builder.addItems(d->m_label.data(), d->m_spinBox.data());
    connect(d->m_spinBox.data(), QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
        d->m_value = value * d->m_displayScaleFactor;
        emit changed();
    });
}

void BaseIntegerAspect::fromMap(const QVariantMap &map)
{
    d->m_value = map.value(settingsKey(), d->m_defaultValue).toLongLong();
}

void BaseIntegerAspect::toMap(QVariantMap &data) const
{
    if (d->m_value != d->m_defaultValue)
        data.insert(settingsKey(), d->m_value);
    else
        data.remove(settingsKey());
}

qint64 BaseIntegerAspect::value() const
{
    return d->m_value;
}

void BaseIntegerAspect::setValue(qint64 value)
{
    d->m_value = value;
    if (d->m_spinBox)
        d->m_spinBox->setValue(int(d->m_value / d->m_displayScaleFactor));
}

void BaseIntegerAspect::setRange(qint64 min, qint64 max)
{
    d->m_minimumValue = min;
    d->m_maximumValue = max;
}

void BaseIntegerAspect::setLabel(const QString &label)
{
    d->m_labelText = label;
    if (d->m_label)
        d->m_label->setText(label);
}

void BaseIntegerAspect::setPrefix(const QString &prefix)
{
    d->m_prefix = prefix;
}

void BaseIntegerAspect::setSuffix(const QString &suffix)
{
    d->m_suffix = suffix;
}

void BaseIntegerAspect::setDisplayIntegerBase(int base)
{
    d->m_displayIntegerBase = base;
}

void BaseIntegerAspect::setDisplayScaleFactor(qint64 factor)
{
    d->m_displayScaleFactor = factor;
}

void BaseIntegerAspect::setEnabled(bool enabled)
{
    d->m_enabled = enabled;
    if (d->m_label)
        d->m_label->setEnabled(enabled);
    if (d->m_spinBox)
        d->m_spinBox->setEnabled(enabled);
}

void BaseIntegerAspect::setDefaultValue(qint64 defaultValue)
{
    d->m_defaultValue = defaultValue;
}

void BaseIntegerAspect::setToolTip(const QString &tooltip)
{
    d->m_tooltip = tooltip;
}

/*!
    \class ProjectExplorer::BaseTristateAspect
*/

BaseTriStateAspect::BaseTriStateAspect()
{
    setDisplayStyle(DisplayStyle::ComboBox);
    setDefaultValue(2);
    addOption(tr("Enable"));
    addOption(tr("Disable"));
    addOption(tr("Leave at Default"));
}

TriState BaseTriStateAspect::setting() const
{
    return TriState::fromVariant(value());
}

void BaseTriStateAspect::setSetting(TriState setting)
{
    setValue(setting.toVariant().toInt());
}

const TriState TriState::Enabled{TriState::EnabledValue};
const TriState TriState::Disabled{TriState::DisabledValue};
const TriState TriState::Default{TriState::DefaultValue};

TriState TriState::fromVariant(const QVariant &variant)
{
    int v = variant.toInt();
    QTC_ASSERT(v == EnabledValue || v == DisabledValue || v == DefaultValue, v = DefaultValue);
    return TriState(Value(v));
}


/*!
    \class ProjectExplorer::BaseStringListAspect
*/

BaseStringListAspect::BaseStringListAspect()
    : d(new Internal::BaseStringListAspectPrivate)
{}

BaseStringListAspect::~BaseStringListAspect() = default;

void BaseStringListAspect::addToLayout(LayoutBuilder &builder)
{
    Q_UNUSED(builder)
    // TODO - when needed.
}

void BaseStringListAspect::fromMap(const QVariantMap &map)
{
    d->m_value = map.value(settingsKey()).toStringList();
}

void BaseStringListAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), d->m_value);
}

QStringList BaseStringListAspect::value() const
{
    return d->m_value;
}

void BaseStringListAspect::setValue(const QStringList &value)
{
    d->m_value = value;
}

} // namespace ProjectExplorer
