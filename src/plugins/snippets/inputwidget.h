/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
