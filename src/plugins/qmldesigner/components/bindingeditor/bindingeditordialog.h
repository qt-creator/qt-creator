// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef BINDINGEDITORDIALOG_H
#define BINDINGEDITORDIALOG_H

#include <bindingeditor/abstracteditordialog.h>

#include <nodemetainfo.h>

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

    void setAllBindings(const QList<BindingOption> &bindings, const NodeMetaInfo &type);

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
    NodeMetaInfo m_type;
};

}

#endif //BINDINGEDITORDIALOG_H
