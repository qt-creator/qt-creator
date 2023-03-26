// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RICHTEXTEDITORDIALOG_H
#define RICHTEXTEDITORDIALOG_H

#include <QDialog>
#include "richtexteditor/richtexteditor.h"
#include "formeditor/formeditoritem.h"

namespace QmlDesigner {


class RichTextEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit RichTextEditorDialog(const QString text);
    void setFormEditorItem(FormEditorItem* formEditorItem);

signals:
    void textChanged(QString text);
private:
    RichTextEditor* m_editor;
    FormEditorItem* m_formEditorItem;

private slots:
    void onTextChanged(QString text);
    void onFinished();
    void setTextToFormEditorItem(QString text);
};

} // namespace QmlDesigner

#endif // RICHTEXTEDITORDIALOG_H
