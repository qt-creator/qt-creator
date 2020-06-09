/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <bindingeditor/bindingeditorwidget.h>
#include <texteditor/texteditor.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class BindingEditorDialog : public QDialog
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

    enum DialogType {
        Unknown = 0,
        BindingDialog = 1,
        ActionDialog = 2
    };

public:
    BindingEditorDialog(QWidget *parent = nullptr, DialogType type = DialogType::BindingDialog);
    ~BindingEditorDialog() override;

    void showWidget();
    void showWidget(int x, int y);

    QString editorValue() const;
    void setEditorValue(const QString &text);

    void setAllBindings(QList<BindingEditorDialog::BindingOption> bindings);
    void adjustProperties();

    void unregisterAutoCompletion();

    QString defaultTitle() const;

private:
    void setupJSEditor();
    void setupUIComponents();
    void setupComboBoxes();

public slots:
    void itemIDChanged(int);
    void propertyIDChanged(int);
    void textChanged();

private:
    DialogType m_dialogType = DialogType::BindingDialog;
    TextEditor::BaseTextEditor *m_editor = nullptr;
    BindingEditorWidget *m_editorWidget = nullptr;
    QVBoxLayout *m_verticalLayout = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QHBoxLayout *m_comboBoxLayout = nullptr;
    QComboBox *m_comboBoxItem = nullptr;
    QComboBox *m_comboBoxProperty = nullptr;
    QList<BindingEditorDialog::BindingOption> m_bindings;
    bool m_lock = false;
    const QString undefinedString = {"[Undefined]"};
    const QString titleString = {tr("Binding Editor")};
};

}

#endif //BINDINGEDITORDIALOG_H
