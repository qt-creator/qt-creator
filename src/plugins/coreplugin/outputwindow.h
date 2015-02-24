/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include "core_global.h"
#include "icontext.h"

#include <utils/outputformat.h>

#include <QPlainTextEdit>

namespace Utils { class OutputFormatter; }

namespace Core {

namespace Internal { class OutputWindowPrivate; }

class CORE_EXPORT OutputWindow : public QPlainTextEdit
{
    Q_OBJECT

public:
    OutputWindow(Context context, QWidget *parent = 0);
    ~OutputWindow();

    Utils::OutputFormatter* formatter() const;
    void setFormatter(Utils::OutputFormatter *formatter);

    void appendMessage(const QString &out, Utils::OutputFormat format);
    /// appends a \p text using \p format without using formater
    void appendText(const QString &text, const QTextCharFormat &format = QTextCharFormat());

    void grayOutOldContent();
    void clear();

    void showEvent(QShowEvent *);

    void scrollToBottom();

    void setMaxLineCount(int count);
    int maxLineCount() const;

public slots:
    void setWordWrapEnabled(bool wrap);

protected:
    bool isScrollbarAtBottom() const;

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *e);
    virtual void keyPressEvent(QKeyEvent *ev);

private:
    void enableUndoRedo();
    QString doNewlineEnforcement(const QString &out);

    Internal::OutputWindowPrivate *d;
};

} // namespace Core

#endif // OUTPUTWINDOW_H
