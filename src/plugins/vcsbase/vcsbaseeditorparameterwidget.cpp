/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "vcsbaseeditorparameterwidget.h"

#include <QtGui/QComboBox>
#include <QtGui/QToolButton>
#include <QtGui/QHBoxLayout>

#include <QtCore/QDebug>

namespace VCSBase {

namespace Internal {

struct SettingMappingData
{
    enum Type
    {
        Invalid,
        Bool,
        String
    };

    SettingMappingData() : boolSetting(0), stringSetting(0)
    { }

    SettingMappingData(bool *setting) : boolSetting(setting), stringSetting(0)
    { }

    SettingMappingData(QString *setting) : boolSetting(0), stringSetting(setting)
    { }

    Type type() const
    {
        if (boolSetting)
            return Bool;
        if (stringSetting)
            return String;
        return Invalid;
    }

    bool *boolSetting;
    QString *stringSetting;
};

} // namespace Internal

VCSBaseEditorParameterWidget::ComboBoxItem::ComboBoxItem()
{
}

VCSBaseEditorParameterWidget::ComboBoxItem::ComboBoxItem(const QString &text,
                                                         const QVariant &val) :
    displayText(text),
    value(val)
{
}

class VCSBaseEditorParameterWidgetPrivate
{
public:
    VCSBaseEditorParameterWidgetPrivate() :
        m_layout(0), m_comboBoxOptionTemplate(QLatin1String("%{option}=%{value}"))
    { }

    QStringList m_baseArguments;
    QHBoxLayout *m_layout;
    QList<VCSBaseEditorParameterWidget::OptionMapping> m_optionMappings;
    QHash<QWidget*, Internal::SettingMappingData> m_settingMapping;
    QStringList m_comboBoxOptionTemplate;
};

/*!
    \class VCSBase::VCSBaseEditorParameterWidget

    \brief A toolbar-like widget for use with VCSBase::VCSBaseEditor::setConfigurationWidget()
    influencing for example the generation of VCS diff output.

    The widget maintains a list of command line arguments (starting from baseArguments())
    which are set according to the state of the inside widgets. A change signal is provided
    that should trigger the rerun of the VCS operation.
*/

VCSBaseEditorParameterWidget::VCSBaseEditorParameterWidget(QWidget *parent) :
    QWidget(parent), d(new VCSBaseEditorParameterWidgetPrivate)
{
    d->m_layout = new QHBoxLayout(this);
    d->m_layout->setContentsMargins(3, 0, 3, 0);
    d->m_layout->setSpacing(2);
    connect(this, SIGNAL(argumentsChanged()), this, SLOT(handleArgumentsChanged()));
}

VCSBaseEditorParameterWidget::~VCSBaseEditorParameterWidget()
{
}

QStringList VCSBaseEditorParameterWidget::baseArguments() const
{
    return d->m_baseArguments;
}

void VCSBaseEditorParameterWidget::setBaseArguments(const QStringList &b)
{
    d->m_baseArguments = b;
}

QStringList VCSBaseEditorParameterWidget::arguments() const
{
    // Compile effective arguments
    QStringList args = baseArguments();
    foreach (const OptionMapping &mapping, optionMappings())
        args += argumentsForOption(mapping);
    return args;
}

QToolButton *VCSBaseEditorParameterWidget::addToggleButton(const QString &option,
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

QComboBox *VCSBaseEditorParameterWidget::addComboBox(const QString &option,
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

void VCSBaseEditorParameterWidget::mapSetting(QToolButton *button, bool *setting)
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

void VCSBaseEditorParameterWidget::mapSetting(QComboBox *comboBox, QString *setting)
{
    if (!d->m_settingMapping.contains(comboBox) && comboBox) {
        d->m_settingMapping.insert(comboBox, Internal::SettingMappingData(setting));
        if (setting) {
            comboBox->blockSignals(true);
            const int itemIndex = comboBox->findData(*setting);
            if (itemIndex != -1)
                comboBox->setCurrentIndex(itemIndex);
            comboBox->blockSignals(false);
        }
    }
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
QStringList VCSBaseEditorParameterWidget::comboBoxOptionTemplate() const
{
    return d->m_comboBoxOptionTemplate;
}

void VCSBaseEditorParameterWidget::setComboBoxOptionTemplate(const QStringList &optTemplate) const
{
    d->m_comboBoxOptionTemplate = optTemplate;
}

void VCSBaseEditorParameterWidget::executeCommand()
{
}

void VCSBaseEditorParameterWidget::handleArgumentsChanged()
{
    updateMappedSettings();
    executeCommand();
}

VCSBaseEditorParameterWidget::OptionMapping::OptionMapping() :
    widget(0)
{
}

VCSBaseEditorParameterWidget::OptionMapping::OptionMapping(const QString &optName, QWidget *w) :
    optionName(optName), widget(w)
{
}

const QList<VCSBaseEditorParameterWidget::OptionMapping> &VCSBaseEditorParameterWidget::optionMappings() const
{
    return d->m_optionMappings;
}

QStringList VCSBaseEditorParameterWidget::argumentsForOption(const OptionMapping &mapping) const
{
    const QToolButton *tb = qobject_cast<const QToolButton *>(mapping.widget);
    if (tb && tb->isChecked())
        return QStringList(mapping.optionName);

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

void VCSBaseEditorParameterWidget::updateMappedSettings()
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
            case Internal::SettingMappingData::Invalid : break;
            } // end switch ()
        }
    }
}

} // namespace VCSBase
