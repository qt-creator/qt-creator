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

#pragma once

#include "vcsbase_global.h"

#include <QVariant>
#include <QToolBar>

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QStringList;
QT_END_NAMESPACE

namespace VcsBase {

class VcsBaseEditorWidget;

namespace Internal { class VcsBaseEditorConfigPrivate; }

// Documentation->inside.
class VCSBASE_EXPORT VcsBaseEditorConfig : public QObject
{
    Q_OBJECT

public:
    explicit VcsBaseEditorConfig(QToolBar *toolBar);
    ~VcsBaseEditorConfig() override;

    class VCSBASE_EXPORT ComboBoxItem
    {
    public:
        ComboBoxItem() = default;
        ComboBoxItem(const QString &text, const QVariant &val);
        QString displayText;
        QVariant value;
    };

    QStringList baseArguments() const;
    void setBaseArguments(const QStringList &);

    QAction *addToggleButton(const QString &option, const QString &label,
                             const QString &tooltip = QString());
    QAction *addToggleButton(const QStringList &options, const QString &label,
                             const QString &tooltip = QString());
    QComboBox *addComboBox(const QStringList &options, const QList<ComboBoxItem> &items);

    void mapSetting(QAction *button, bool *setting);
    void mapSetting(QComboBox *comboBox, QString *setting);
    void mapSetting(QComboBox *comboBox, int *setting);

    // Return the effective arguments according to setting.
    virtual QStringList arguments() const;

public slots:
    void handleArgumentsChanged();
    void executeCommand();

signals:
    void commandExecutionRequested();

    // Trigger a re-run to show changed output according to new argument list.
    void argumentsChanged();

protected:
    class OptionMapping
    {
    public:
        OptionMapping() = default;
        OptionMapping(const QString &option, QObject *obj);
        OptionMapping(const QStringList &optionList, QObject *obj);
        QStringList options;
        QObject *object = nullptr;
    };

    const QList<OptionMapping> &optionMappings() const;
    virtual QStringList argumentsForOption(const OptionMapping &mapping) const;
    void updateMappedSettings();

private:
    friend class Internal::VcsBaseEditorConfigPrivate;
    Internal::VcsBaseEditorConfigPrivate *const d;
};

} // namespace VcsBase
