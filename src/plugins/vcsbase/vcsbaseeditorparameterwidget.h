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

#ifndef VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H
#define VCSBASE_VCSBASEEDITORPARAMETERWIDGET_H

#include "vcsbase_global.h"

#include <QStringList>
#include <QVariant>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
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
    QComboBox *addComboBox(const QString &option, const QList<ComboBoxItem> &items);

    void mapSetting(QToolButton *button, bool *setting);
    void mapSetting(QComboBox *comboBox, QString *setting);
    void mapSetting(QComboBox *comboBox, int *setting);

    QStringList comboBoxOptionTemplate() const;
    void setComboBoxOptionTemplate(const QStringList &optTemplate) const;

    // Return the effective arguments according to setting.
    virtual QStringList arguments() const;

public slots:
    virtual void executeCommand();
    virtual void handleArgumentsChanged();

signals:
    // Trigger a re-run to show changed output according to new argument list.
    void argumentsChanged();

protected:
    struct OptionMapping
    {
        OptionMapping();
        OptionMapping(const QString &optName, QWidget *w);
        QString optionName;
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
