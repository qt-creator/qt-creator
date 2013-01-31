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

#include "vcsbaseeditorparameterwidget.h"

#include <QComboBox>
#include <QToolButton>
#include <QHBoxLayout>

#include <QDebug>

namespace VcsBase {

namespace Internal {

class SettingMappingData
{
public:
    enum Type
    {
        Invalid,
        Bool,
        String,
        Int
    };

    SettingMappingData() : boolSetting(0), m_type(Invalid)
    { }

    SettingMappingData(bool *setting) : boolSetting(setting), m_type(Bool)
    { }

    SettingMappingData(QString *setting) : stringSetting(setting), m_type(String)
    { }

    SettingMappingData(int *setting) : intSetting(setting), m_type(Int)
    { }

    Type type() const
    {
        return m_type;
    }

    union {
        bool *boolSetting;
        QString *stringSetting;
        int *intSetting;
    };

private:
    Type m_type;
};

class VcsBaseEditorParameterWidgetPrivate
{
public:
    VcsBaseEditorParameterWidgetPrivate() :
        m_layout(0), m_comboBoxOptionTemplate(QLatin1String("%{option}=%{value}"))
    { }

    QStringList m_baseArguments;
    QHBoxLayout *m_layout;
    QList<VcsBaseEditorParameterWidget::OptionMapping> m_optionMappings;
    QHash<QWidget*, SettingMappingData> m_settingMapping;
    QStringList m_comboBoxOptionTemplate;
};

} // namespace Internal

/*!
    \class VcsBase::VcsBaseEditorParameterWidget

    \brief A toolbar-like widget for use with VcsBase::VcsBaseEditor::setConfigurationWidget()
    influencing for example the generation of VCS diff output.

    The widget maintains a list of command line arguments (starting from baseArguments())
    which are set according to the state of the inside widgets. A change signal is provided
    that should trigger the rerun of the VCS operation.
*/

VcsBaseEditorParameterWidget::ComboBoxItem::ComboBoxItem(const QString &text,
                                                         const QVariant &val) :
    displayText(text),
    value(val)
{
}

VcsBaseEditorParameterWidget::VcsBaseEditorParameterWidget(QWidget *parent) :
    QWidget(parent), d(new Internal::VcsBaseEditorParameterWidgetPrivate)
{
    d->m_layout = new QHBoxLayout(this);
    d->m_layout->setContentsMargins(3, 0, 3, 0);
    d->m_layout->setSpacing(2);
    connect(this, SIGNAL(argumentsChanged()), this, SLOT(handleArgumentsChanged()));
}

VcsBaseEditorParameterWidget::~VcsBaseEditorParameterWidget()
{
    delete d;
}

QStringList VcsBaseEditorParameterWidget::baseArguments() const
{
    return d->m_baseArguments;
}

void VcsBaseEditorParameterWidget::setBaseArguments(const QStringList &b)
{
    d->m_baseArguments = b;
}

QStringList VcsBaseEditorParameterWidget::arguments() const
{
    // Compile effective arguments
    QStringList args = baseArguments();
    foreach (const OptionMapping &mapping, optionMappings())
        args += argumentsForOption(mapping);
    return args;
}

QToolButton *VcsBaseEditorParameterWidget::addToggleButton(const QString &option,
                                                           const QString &label,
                                                           const QString &toolTip)
{
    QToolButton *tb = new QToolButton;
    tb->setText(label);
    tb->setToolTip(toolTip);
    tb->setCheckable(true);
    connect(tb, SIGNAL(toggled(bool)), this, SIGNAL(argumentsChanged()));
    d->m_layout->addWidget(tb);
    d->m_optionMappings.append(OptionMapping(option, tb));
    return tb;
}

QComboBox *VcsBaseEditorParameterWidget::addComboBox(const QString &option,
                                                     const QList<ComboBoxItem> &items)
{
    QComboBox *cb = new QComboBox;
    foreach (const ComboBoxItem &item, items)
        cb->addItem(item.displayText, item.value);
    connect(cb, SIGNAL(currentIndexChanged(int)), this, SIGNAL(argumentsChanged()));
    d->m_layout->addWidget(cb);
    d->m_optionMappings.append(OptionMapping(option, cb));
    return cb;
}

void VcsBaseEditorParameterWidget::mapSetting(QToolButton *button, bool *setting)
{
    if (!d->m_settingMapping.contains(button) && button) {
        d->m_settingMapping.insert(button, Internal::SettingMappingData(setting));
        if (setting) {
            button->blockSignals(true);
            button->setChecked(*setting);
            button->blockSignals(false);
        }
    }
}

void VcsBaseEditorParameterWidget::mapSetting(QComboBox *comboBox, QString *setting)
{
    if (!d->m_settingMapping.contains(comboBox) && comboBox) {
        d->m_settingMapping.insert(comboBox, Internal::SettingMappingData(setting));
        if (setting) {
            comboBox->blockSignals(true);
            const int itemIndex = setting ? comboBox->findData(*setting) : -1;
            if (itemIndex != -1)
                comboBox->setCurrentIndex(itemIndex);
            comboBox->blockSignals(false);
        }
    }
}

void VcsBaseEditorParameterWidget::mapSetting(QComboBox *comboBox, int *setting)
{
    if (d->m_settingMapping.contains(comboBox) || !comboBox)
        return;

    d->m_settingMapping.insert(comboBox, Internal::SettingMappingData(setting));

    if (!setting || *setting < 0 || *setting >= comboBox->count())
        return;

    comboBox->blockSignals(true);
    comboBox->setCurrentIndex(*setting);
    comboBox->blockSignals(false);
}

/*!
    \brief This property holds the format (template) of assignable command line
    options (like --file=<file> for example)

    The option's name and its actual value are specified with place markers
    within the template :
      \li %{option} for the option
      \li %{value} for the actual value

    \code
    QStringList("%{option}=%{value}"); // eg --file=a.out
    QStringList() << "%{option}" << "%{value}"; // eg --file a.out (two distinct arguments)
    \endcode
*/
QStringList VcsBaseEditorParameterWidget::comboBoxOptionTemplate() const
{
    return d->m_comboBoxOptionTemplate;
}

void VcsBaseEditorParameterWidget::setComboBoxOptionTemplate(const QStringList &optTemplate) const
{
    d->m_comboBoxOptionTemplate = optTemplate;
}

void VcsBaseEditorParameterWidget::executeCommand()
{
}

void VcsBaseEditorParameterWidget::handleArgumentsChanged()
{
    updateMappedSettings();
    executeCommand();
}

VcsBaseEditorParameterWidget::OptionMapping::OptionMapping() :
    widget(0)
{
}

VcsBaseEditorParameterWidget::OptionMapping::OptionMapping(const QString &optName, QWidget *w) :
    optionName(optName), widget(w)
{
}

const QList<VcsBaseEditorParameterWidget::OptionMapping> &VcsBaseEditorParameterWidget::optionMappings() const
{
    return d->m_optionMappings;
}

QStringList VcsBaseEditorParameterWidget::argumentsForOption(const OptionMapping &mapping) const
{
    const QToolButton *tb = qobject_cast<const QToolButton *>(mapping.widget);
    if (tb && tb->isChecked()) {
        if (!mapping.optionName.isEmpty())
            return QStringList(mapping.optionName);
        else
            return QStringList();
    }

    const QComboBox *cb = qobject_cast<const QComboBox *>(mapping.widget);
    if (cb) {
        const QString value = cb->itemData(cb->currentIndex()).toString();
        QStringList args;
        foreach (const QString &t, d->m_comboBoxOptionTemplate) {
            QString a = t;
            a.replace(QLatin1String("%{option}"), mapping.optionName);
            a.replace(QLatin1String("%{value}"), value);
            args += a;
        }
        return args;
    }

    return QStringList();
}

void VcsBaseEditorParameterWidget::updateMappedSettings()
{
    foreach (const OptionMapping &optMapping, d->m_optionMappings) {
        if (d->m_settingMapping.contains(optMapping.widget)) {
            Internal::SettingMappingData& settingData = d->m_settingMapping[optMapping.widget];
            switch (settingData.type()) {
            case Internal::SettingMappingData::Bool :
            {
                const QToolButton *tb = qobject_cast<const QToolButton *>(optMapping.widget);
                if (tb)
                    *settingData.boolSetting = tb->isChecked();
                break;
            }
            case Internal::SettingMappingData::String :
            {
                const QComboBox *cb = qobject_cast<const QComboBox *>(optMapping.widget);
                if (cb && cb->currentIndex() != -1)
                    *settingData.stringSetting = cb->itemData(cb->currentIndex()).toString();
                break;
            }
            case Internal::SettingMappingData::Int:
            {
                const QComboBox *cb = qobject_cast<const QComboBox *>(optMapping.widget);
                if (cb && cb->currentIndex() != -1)
                    *settingData.intSetting = cb->currentIndex();
                break;
            }
            case Internal::SettingMappingData::Invalid : break;
            } // end switch ()
        }
    }
}

} // namespace VcsBase
