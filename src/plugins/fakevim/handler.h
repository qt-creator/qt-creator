/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef FAKEVIM_HANDLER_H
#define FAKEVIM_HANDLER_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QString;
class QEvent;
QT_END_NAMESPACE

namespace FakeVim {
namespace Internal {

class FakeVimHandler : public QObject
{
    Q_OBJECT

public:
    FakeVimHandler(QObject *parent = 0);
    ~FakeVimHandler();

public slots:
    // The same handler can be installed on several widgets
    // FIXME: good idea?
    void addWidget(QWidget *widget);
    void removeWidget(QWidget *widget);

    // This executes an "ex" style command taking context
    // information from \p widget;
    void handleCommand(QWidget *widget, const QString &cmd);
    void quit();

signals:
    void commandBufferChanged(const QString &msg);
    void statusDataChanged(const QString &msg);
    void quitRequested(QWidget *);

private:
    bool eventFilter(QObject *ob, QEvent *ev);

    class Private;
    friend class Private;
    Private *d;
};

} // namespace Internal
} // namespace FakeVim

#endif // FAKEVIM_H
