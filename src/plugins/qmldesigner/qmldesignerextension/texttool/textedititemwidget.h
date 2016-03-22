/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    ~TextEditItemWidget();

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
