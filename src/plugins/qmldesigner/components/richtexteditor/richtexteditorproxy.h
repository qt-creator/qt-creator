// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QQuickItem>

QT_BEGIN_NAMESPACE
class QDialog;
QT_END_NAMESPACE

namespace QmlDesigner {

class RichTextEditor;

class RichTextEditorProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString richText READ richText WRITE setRichText)
public:
    explicit RichTextEditorProxy(QObject *parent = nullptr);
    ~RichTextEditorProxy();

    static void registerDeclarativeType();

    Q_INVOKABLE void showWidget();
    Q_INVOKABLE void hideWidget();

    QString richText() const;
    void setRichText(const QString &text);

signals:
    void accepted();
    void rejected();

private:
    QDialog *m_dialog;
    RichTextEditor *m_widget;
};

} // namespace QmlDesigner
