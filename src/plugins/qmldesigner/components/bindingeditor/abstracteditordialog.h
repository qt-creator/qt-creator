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

#ifndef ABSTRACTEDITORDIALOG_H
#define ABSTRACTEDITORDIALOG_H

#include <qmldesignercorelib_global.h>

#include <texteditor/texteditor.h>
#include <bindingeditor/bindingeditorwidget.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QVBoxLayout;
class QHBoxLayout;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractEditorDialog : public QDialog
{
    Q_OBJECT

public:
    AbstractEditorDialog(QWidget *parent = nullptr, const QString &title = tr("Untitled Editor"));
    ~AbstractEditorDialog() override;

    void showWidget();
    void showWidget(int x, int y);

    QString editorValue() const;
    void setEditorValue(const QString &text);

    virtual void adjustProperties()= 0;

    void unregisterAutoCompletion();

    QString defaultTitle() const;

    BindingEditorWidget *bindingEditorWidget() const
    {
        return m_editorWidget;
    }

protected:
    void setupJSEditor();
    void setupUIComponents();

    static bool isNumeric(const TypeName &type);
    static bool isColor(const TypeName &type);
    static bool isVariant(const TypeName &type);

public slots:
    void textChanged();

protected:
    TextEditor::BaseTextEditor *m_editor = nullptr;
    BindingEditorWidget *m_editorWidget = nullptr;
    QVBoxLayout *m_verticalLayout = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QHBoxLayout *m_comboBoxLayout = nullptr;
    bool m_lock = false;
    QString m_titleString;

    const QString undefinedString = {"[Undefined]"};
};

}

#endif //ABSTRACTEDITORDIALOG_H
