/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CONSOLEEDITOR_H
#define CONSOLEEDITOR_H

#include "consoleitemmodel.h"

#include <QtGui/QTextEdit>
#include <QtCore/QModelIndex>

namespace Debugger {
namespace Internal {

class ConsoleBackend;
class ConsoleEditor : public QTextEdit
{
    Q_OBJECT
public:
    explicit ConsoleEditor(const QModelIndex &index,
                           ConsoleBackend *backend = 0,
                           QWidget *parent = 0);

    QString getCurrentScript() const;

protected:
    void keyPressEvent(QKeyEvent *e);
    void contextMenuEvent(QContextMenuEvent *event);
    void focusOutEvent(QFocusEvent *e);

signals:
    void editingFinished();
    void appendEditableRow();

protected:
    void handleUpKey();
    void handleDownKey();

    void replaceCurrentScript(const QString &script);

private:
    ConsoleBackend *m_consoleBackend;
    QModelIndex m_historyIndex;
    QString m_cachedScript;
    QImage m_prompt;
    int m_startOfEditableArea;
};

} //Internal
} //Debugger

#endif // CONSOLEEDITOR_H
