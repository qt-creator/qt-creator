/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
