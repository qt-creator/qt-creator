/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef COMPLETINGTEXTEDIT_H
#define COMPLETINGTEXTEDIT_H

#include "utils_global.h"

#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QCompleter;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT CompletingTextEdit : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int completionLengthThreshold
               READ completionLengthThreshold WRITE setCompletionLengthThreshold)

public:
    CompletingTextEdit(QWidget *parent = 0);
    ~CompletingTextEdit();

    void setCompleter(QCompleter *c);
    QCompleter *completer() const;

    int completionLengthThreshold() const;
    void setCompletionLengthThreshold(int len);

protected:
    void keyPressEvent(QKeyEvent *e);
    void focusInEvent(QFocusEvent *e);

private:
    class CompletingTextEditPrivate *d;
    Q_PRIVATE_SLOT(d, void insertCompletion(const QString &))
};

} // namespace Utils

#endif // COMPLETINGTEXTEDIT_H
