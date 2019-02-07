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

#include <utils/utilsicons.h>
#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QSpinBox>
#include <QToolButton>
#include <QTextEdit>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class BaseBoolAspectPrivate
{
public:
    bool m_value = false;
    bool m_defaultValue = false;
    QString m_label;
    QString m_tooltip;
    QPointer<QCheckBox> m_checkBox; // Owned by configuration widget
};

class BaseStringAspectPrivate
{
public:
    BaseStringAspect::DisplayStyle m_displayStyle = BaseStringAspect::LabelDisplay;
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
    QPixmap m_labelPixmap;
};

class BaseIntegerAspectPrivate
{
public:
    QVariant m_value;
    QVariant m_minimumValue;
    QVariant m_maximumValue;
    int m_displayIntegerBase = 10;
    QString m_label;
    QString m_prefix;
    QString m_suffix;
    QPointer<QSpinBox> m_spinBox; // Owned by configuration widget
};

} // Internal

/*!
    \class ProjectExplorer::BaseStringAspect
*/

BaseStringAspect::BaseStringAspect()
    : d(new Internal::BaseStringAspectPrivate)
{}

BaseStringAspect::~BaseStringAspect() = default;

QString BaseStringAspect::value() const
{
    return d->m_value;
}

void BaseStringAspect::setValue(const QString &value)
{
    const bool isSame = value == d->m_value;
    d->m_value = value;
    update();
    if (!isSame)
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

FileName BaseStringAspect::fileName() const
{
    return FileName::fromString(d->m_value);
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

void BaseStringAspect::setEnvironment(const Environment &env)
{
    d->m_environment = env;
    if (d->m_pathChooserDisplay)
        d->m_pathChooserDisplay->setEnvironment(env);
}

void BaseStringAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!d->m_label);
    QWidget *parent = layout->parentWidget();
    d->m_label = new QLabel(parent);
    d->m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    d->m_label->setText(d->m_labelText);
    if (!d->m_labelPixmap.isNull())
        d->m_label->setPixmap(d->m_labelPixmap);

    auto hbox = new QHBoxLayout;
    switch (d->m_displayStyle) {
    case PathChooserDisplay:
        d->m_pathChooserDisplay = new PathChooser(parent);
        d->m_pathChooserDisplay->setExpectedKind(d->m_expectedKind);
        if (!d->m_historyCompleterKey.isEmpty())
            d->m_pathChooserDisplay->setHistoryCompleter(d->m_historyCompleterKey);
        d->m_pathChooserDisplay->setEnvironment(d->m_environment);
        connect(d->m_pathChooserDisplay, &PathChooser::pathChanged,
                this, &BaseStringAspect::setValue);
        hbox->addWidget(d->m_pathChooserDisplay);
        break;
    case LineEditDisplay:
        d->m_lineEditDisplay = new FancyLineEdit(parent);
        d->m_lineEditDisplay->setPlaceholderText(d->m_placeHolderText);
        if (!d->m_historyCompleterKey.isEmpty())
            d->m_lineEditDisplay->setHistoryCompleter(d->m_historyCompleterKey);
        connect(d->m_lineEditDisplay, &FancyLineEdit::textEdited,
                this, &BaseStringAspect::setValue);
        hbox->addWidget(d->m_lineEditDisplay);
        break;
    case TextEditDisplay:
        d->m_textEditDisplay = new QTextEdit(parent);
        d->m_textEditDisplay->setPlaceholderText(d->m_placeHolderText);
        connect(d->m_textEditDisplay, &QTextEdit::textChanged, this, [this] {
            const QString value = d->m_textEditDisplay->document()->toPlainText();
            if (value != d->m_value) {
                d->m_value = value;
                emit changed();
            }
        });
        hbox->addWidget(d->m_textEditDisplay);
        break;
    case LabelDisplay:
        d->m_labelDisplay = new QLabel(parent);
        d->m_labelDisplay->setTextInteractionFlags(Qt::TextSelectableByMouse);
        hbox->addWidget(d->m_labelDisplay);
        break;
    }

    if (d->m_checker) {
        auto form = new QFormLayout;
        form->setContentsMargins(0, 0, 0, 0);
        form->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        d->m_checker->addToConfigurationLayout(form);
        hbox->addLayout(form);
    }
    layout->addRow(d->m_label, hbox);

    update();
}

void BaseStringAspect::update()
{
    const QString displayedString = d->m_displayFilter ? d->m_displayFilter(d->m_value)
                                                       : d->m_value;

    const bool enabled = !d->m_checker || d->m_checker->value();

    if (d->m_pathChooserDisplay) {
        d->m_pathChooserDisplay->setFileName(FileName::fromString(displayedString));
        d->m_pathChooserDisplay->setEnabled(enabled);
    }

    if (d->m_lineEditDisplay) {
        d->m_lineEditDisplay->setTextKeepingActiveCursor(displayedString);
        d->m_lineEditDisplay->setEnabled(enabled);
    }

    if (d->m_textEditDisplay) {
        d->m_textEditDisplay->setText(displayedString);
        d->m_textEditDisplay->setEnabled(enabled);
    }

    if (d->m_labelDisplay)
        d->m_labelDisplay->setText(displayedString);

    if (d->m_label) {
        d->m_label->setText(d->m_labelText);
        if (!d->m_labelPixmap.isNull())
            d->m_label->setPixmap(d->m_labelPixmap);
    }
}

void BaseStringAspect::makeCheckable(const QString &checkerLabel, const QString &checkerKey)
{
    QTC_ASSERT(!d->m_checker, return);
    d->m_checker.reset(new BaseBoolAspect);
    d->m_checker->setLabel(checkerLabel);
    d->m_checker->setSettingsKey(checkerKey);

    connect(d->m_checker.get(), &BaseBoolAspect::changed, this, &BaseStringAspect::update);
    connect(d->m_checker.get(), &BaseBoolAspect::changed, this, &BaseStringAspect::changed);

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

void BaseBoolAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!d->m_checkBox);
    d->m_checkBox = new QCheckBox(d->m_label, layout->parentWidget());
    d->m_checkBox->setChecked(d->m_value);
    d->m_checkBox->setToolTip(d->m_tooltip);
    layout->addRow(QString(), d->m_checkBox);
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

void BaseBoolAspect::setLabel(const QString &label)
{
    d->m_label = label;
}

void BaseBoolAspect::setToolTip(const QString &tooltip)
{
    d->m_tooltip = tooltip;
}

/*!
    \class ProjectExplorer::BaseIntegerAspect
*/

// BaseIntegerAspect

BaseIntegerAspect::BaseIntegerAspect()
    : d(new Internal::BaseIntegerAspectPrivate)
{}

BaseIntegerAspect::~BaseIntegerAspect() = default;

void BaseIntegerAspect::addToConfigurationLayout(QFormLayout *layout)
{
    QTC_CHECK(!d->m_spinBox);
    d->m_spinBox = new QSpinBox(layout->parentWidget());
    d->m_spinBox->setValue(d->m_value.toInt());
    d->m_spinBox->setDisplayIntegerBase(d->m_displayIntegerBase);
    d->m_spinBox->setPrefix(d->m_prefix);
    d->m_spinBox->setSuffix(d->m_suffix);
    if (d->m_maximumValue.isValid() && d->m_maximumValue.isValid())
        d->m_spinBox->setRange(d->m_minimumValue.toInt(), d->m_maximumValue.toInt());
    layout->addRow(d->m_label, d->m_spinBox);
    connect(d->m_spinBox.data(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this](int value) {
        d->m_value = value;
        emit changed();
    });
}

void BaseIntegerAspect::fromMap(const QVariantMap &map)
{
    d->m_value = map.value(settingsKey());
}

void BaseIntegerAspect::toMap(QVariantMap &data) const
{
    data.insert(settingsKey(), d->m_value);
}

int BaseIntegerAspect::value() const
{
    return d->m_value.toInt();
}

void BaseIntegerAspect::setValue(int value)
{
    d->m_value = value;
    if (d->m_spinBox)
        d->m_spinBox->setValue(d->m_value.toInt());
}

void BaseIntegerAspect::setRange(int min, int max)
{
    d->m_minimumValue = min;
    d->m_maximumValue = max;
}

void BaseIntegerAspect::setLabel(const QString &label)
{
    d->m_label = label;
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

} // namespace ProjectExplorer
