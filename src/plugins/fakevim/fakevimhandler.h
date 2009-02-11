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

#ifndef FAKEVIM_HANDLER_H
#define FAKEVIM_HANDLER_H

#include <QtCore/QObject>
#include <QtGui/QTextEdit>
#include <QtGui/QTextBlock>

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
    FakeVimHandler(QWidget *widget, QObject *parent = 0);
    ~FakeVimHandler();

    QWidget *widget();

    void setExtraData(QObject *data);
    QObject *extraData() const;

public slots:
    void setCurrentFileName(const QString &fileName);

    // This executes an "ex" style command taking context
    // information from widget;
    void handleCommand(const QString &cmd);
    void setConfigValue(const QString &key, const QString &value);
    void quit();

    // Convenience
    void setupWidget();
    void restoreWidget();

signals:
    void commandBufferChanged(const QString &msg);
    void statusDataChanged(const QString &msg);
    void extraInformationChanged(const QString &msg);
    void quitRequested();
    void selectionChanged(const QList<QTextEdit::ExtraSelection> &selection);
    void writeFileRequested(bool *handled,
        const QString &fileName, const QString &contents);
    void moveToMatchingParenthesis(bool *moved, bool *forward, QTextCursor *cursor);
    void indentRegion(int *amount, QTextBlock begin, QTextBlock end, QChar typedChar);

private:
    bool eventFilter(QObject *ob, QEvent *ev);

    class Private;
    friend class Private;
    Private *d;
};

} // namespace Internal
} // namespace FakeVim

#endif // FAKEVIM_H
