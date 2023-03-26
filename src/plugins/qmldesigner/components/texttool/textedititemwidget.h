// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QGraphicsProxyWidget>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QLineEdit;
class QGraphicsScene;
QT_END_NAMESPACE

namespace QmlDesigner {

class TextEditItemWidget : public QGraphicsProxyWidget
{
    Q_OBJECT
public:
    TextEditItemWidget(QGraphicsScene *scene);
    ~TextEditItemWidget() override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void activateTextEdit(const QSize &maximumSize);
    void activateLineEdit();
    void updateText(const QString &text);

protected:
    QLineEdit* lineEdit() const;
    QTextEdit* textEdit() const;

    QString text() const;
private:
    mutable QScopedPointer<QLineEdit> m_lineEdit;
    mutable QScopedPointer<QTextEdit> m_textEdit;
};
} // namespace QmlDesigner
