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

#ifndef FAKEVIM_HANDLER_H
#define FAKEVIM_HANDLER_H

#include <QtCore/QObject>
#include <QtGui/QTextEdit>

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

private:
    bool eventFilter(QObject *ob, QEvent *ev);

    class Private;
    friend class Private;
    Private *d;
};

} // namespace Internal
} // namespace FakeVim

#endif // FAKEVIM_H
