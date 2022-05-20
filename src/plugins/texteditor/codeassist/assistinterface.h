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

#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QString>
#include <QTextCursor>
#include <QVector>

#include <utils/filepath.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT AssistInterface
{
public:
    AssistInterface(const QTextCursor &cursor,
                    const Utils::FilePath &filePath,
                    AssistReason reason);
    virtual ~AssistInterface();

    virtual int position() const { return m_position; }
    virtual QChar characterAt(int position) const;
    virtual QString textAt(int position, int length) const;
    QTextCursor cursor() const { return m_cursor; }
    virtual Utils::FilePath filePath() const { return m_filePath; }
    virtual QTextDocument *textDocument() const { return m_textDocument; }
    virtual void prepareForAsyncUse();
    virtual void recreateTextDocument();
    virtual AssistReason reason() const;

private:
    QTextDocument *m_textDocument;
    QTextCursor m_cursor;
    bool m_isAsync;
    int m_position;
    int m_anchor;
    Utils::FilePath m_filePath;
    AssistReason m_reason;
    QString m_text;
    QVector<int> m_userStates;
};

} // namespace TextEditor
