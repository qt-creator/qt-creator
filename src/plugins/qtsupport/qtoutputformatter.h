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

#include "qtsupport_global.h"

#include <utils/outputformatter.h>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace ProjectExplorer { class Project; }

namespace QtSupport {

struct LinkResult
{
    int start;
    int end;
    QString href;
};

namespace Internal {
class QtOutputFormatterPrivate;
class QtSupportPlugin;
}

class QTSUPPORT_EXPORT QtOutputFormatter : public Utils::OutputFormatter
{
    Q_OBJECT
public:
    explicit QtOutputFormatter(ProjectExplorer::Project *project);
    ~QtOutputFormatter();

    void appendMessage(const QString &text, Utils::OutputFormat format);
    void appendMessage(const QString &text, const QTextCharFormat &format);
    void handleLink(const QString &href);
    void setPlainTextEdit(QPlainTextEdit *plainText);

protected:
    void clearLastLine();
    virtual void openEditor(const QString &fileName, int line, int column = -1);

private:
    void updateProjectFileList();
    LinkResult matchLine(const QString &line) const;
    void appendMessagePart(QTextCursor &cursor, const QString &txt, const QTextCharFormat &format);
    void appendLine(QTextCursor &cursor, const LinkResult &lr, const QString &line,
                    Utils::OutputFormat);
    void appendLine(QTextCursor &cursor, const LinkResult &lr, const QString &line,
                    const QTextCharFormat &format);

    Internal::QtOutputFormatterPrivate *d;

    // for testing
    friend class Internal::QtSupportPlugin;
};


} // namespace QtSupport
