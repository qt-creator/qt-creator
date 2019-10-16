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

#ifndef BINDINGEDITOR_H
#define BINDINGEDITOR_H

#include "texteditorview.h"
#include <texteditor/texteditor.h>
#include <QtQml>

#include <QWidget>
#include <QDialog>
#include <QPointer>

#include <qmljseditor/qmljseditor.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QDialogButtonBox;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class BindingEditorContext : public Core::IContext
{
    Q_OBJECT

public:
    BindingEditorContext(QWidget *parent) : Core::IContext(parent)
    {
        setWidget(parent);
    }
};

class BindingEditorWidget : public QmlJSEditor::QmlJSEditorWidget
{
    Q_OBJECT

public:
    BindingEditorWidget();
    ~BindingEditorWidget() override;

    void unregisterAutoCompletion();

    bool event(QEvent *event) override;

    TextEditor::AssistInterface *createAssistInterface(TextEditor::AssistKind assistKind,
            TextEditor::AssistReason assistReason) const override;

signals:
    void returnKeyClicked();

public:
    QmlJSEditor::QmlJSEditorDocument *qmljsdocument = nullptr;
    BindingEditorContext *m_context = nullptr;
    QAction *m_completionAction = nullptr;
};

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

public:
    BindingEditorDialog(QWidget *parent = nullptr);
    ~BindingEditorDialog() override;

    void showWidget(int x, int y);

    QString editorValue() const;
    void setEditorValue(const QString &text);

    void setAllBindings(QList<BindingEditorDialog::BindingOption> bindings);
    void adjustProperties();

    void unregisterAutoCompletion();

private:
    void setupJSEditor();
    void setupUIComponents();
    void setupComboBoxes();

public slots:
    void itemIDChanged(int);
    void propertyIDChanged(int);
    void textChanged();

private:
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
};

class BindingEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ bindingValue WRITE setBindingValue)
    Q_PROPERTY(QVariant backendValueProperty READ backendValue WRITE setBackendValue NOTIFY backendValueChanged)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)

public:
    BindingEditor(QObject *parent = nullptr);
    ~BindingEditor();

    static void registerDeclarativeType();

    Q_INVOKABLE void showWidget(int x, int y);
    Q_INVOKABLE void hideWidget();

    QString bindingValue() const;
    void setBindingValue(const QString &text);

    void setBackendValue(const QVariant &backendValue);
    void setModelNodeBackend(const QVariant &modelNodeBackend);

    Q_INVOKABLE void prepareBindings();

signals:
    void accepted();
    void rejected();
    void backendValueChanged();
    void modelNodeBackendChanged();

private:
    QVariant backendValue() const;
    QVariant modelNodeBackend() const;

private:
    QPointer<BindingEditorDialog> m_dialog;
    QVariant m_backendValue;
    QVariant m_modelNodeBackend;
    TypeName m_backendValueTypeName;
};

}

QML_DECLARE_TYPE(QmlDesigner::BindingEditor)

#endif //BINDINGEDITOR_H
