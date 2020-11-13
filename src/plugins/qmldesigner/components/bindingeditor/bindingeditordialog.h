/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef BINDINGEDITORDIALOG_H
#define BINDINGEDITORDIALOG_H

#include <bindingeditor/abstracteditordialog.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class BindingEditorDialog : public AbstractEditorDialog
{
    Q_OBJECT

public:
    struct BindingOption
    {
        BindingOption() {}
        BindingOption(const QString &value) { item = value; }

        bool operator==(const QString &value) const { return value == item; }
        bool operator==(const BindingOption &value) const { return value.item == item; }

        QString item;
        QStringList properties;
    };

    BindingEditorDialog(QWidget *parent = nullptr);
    ~BindingEditorDialog() override;

    void adjustProperties() override;

    void setAllBindings(const QList<BindingOption> &bindings, const TypeName &type);

private:
    void setupUIComponents();
    void setupComboBoxes();
    void setupCheckBox();

public slots:
    void itemIDChanged(int);
    void propertyIDChanged(int);
    void checkBoxChanged(int);

private:
    QComboBox *m_comboBoxItem = nullptr;
    QComboBox *m_comboBoxProperty = nullptr;
    QCheckBox *m_checkBoxNot = nullptr;

    QList<BindingOption> m_bindings;
    TypeName m_type;
};

}

#endif //BINDINGEDITORDIALOG_H
