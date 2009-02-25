/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef INPUTWIDGET_H
#define INPUTWIDGET_H

#include <QtGui/QFrame>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
QT_END_NAMESPACE

namespace Snippets {
namespace Internal {

class InputWidget : public QFrame
{
    Q_OBJECT

public:
    InputWidget(const QString &text, const QString &value);
    void showInputWidget(const QPoint &position);

signals:
    void finished(bool canceled, const QString &value);

protected:
    bool eventFilter(QObject *, QEvent *);
    void resizeEvent(QResizeEvent *event);

private:
    void closeInputWidget(bool cancel);
    QLabel *m_label;
    QLineEdit *m_lineEdit;
};

} // namespace Internal
} // namespace Snippets

#endif // INPUTWIDGET_H
