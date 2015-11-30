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

#ifndef VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H
#define VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H

#include "vcsbase_global.h"

#include <QVariant>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QStringList;
QT_END_NAMESPACE

namespace VcsBase {

namespace Internal { class VcsBaseEditorParameterWidgetPrivate; }

// Documentation->inside.
class VCSBASE_EXPORT VcsBaseEditorParameterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VcsBaseEditorParameterWidget(QWidget *parent = 0);
    ~VcsBaseEditorParameterWidget();

    struct VCSBASE_EXPORT ComboBoxItem
    {
        ComboBoxItem() {}
        ComboBoxItem(const QString &text, const QVariant &val);
        QString displayText;
        QVariant value;
    };

    QStringList baseArguments() const;
    void setBaseArguments(const QStringList &);

    QToolButton *addToggleButton(const QString &option, const QString &label,
                                 const QString &tooltip = QString());
    QToolButton *addToggleButton(const QStringList &options, const QString &label,
                                 const QString &tooltip = QString());
    QComboBox *addComboBox(const QStringList &options, const QList<ComboBoxItem> &items);

    void mapSetting(QToolButton *button, bool *setting);
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
    struct OptionMapping
    {
        OptionMapping();
        OptionMapping(const QString &option, QWidget *w);
        OptionMapping(const QStringList &optionList, QWidget *w);
        QStringList options;
        QWidget *widget;
    };

    const QList<OptionMapping> &optionMappings() const;
    virtual QStringList argumentsForOption(const OptionMapping &mapping) const;
    void updateMappedSettings();

private:
    friend class Internal::VcsBaseEditorParameterWidgetPrivate;
    Internal::VcsBaseEditorParameterWidgetPrivate *const d;
};

} // namespace VcsBase

#endif // VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H
