// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QString>
#include <QTextCursor>

#include <utils/filepath.h>

namespace TextEditor {

class TEXTEDITOR_EXPORT AssistInterface
{
public:
    AssistInterface(const QTextCursor &cursor, const Utils::FilePath &filePath,
                    AssistReason reason, bool isBaseObject = true);
    virtual ~AssistInterface();

    int position() const { return m_position; }
    QChar characterAt(int position) const;
    QString textAt(int position, int length) const;
    QTextCursor cursor() const { return m_cursor; }
    Utils::FilePath filePath() const { return m_filePath; }
    QTextDocument *textDocument() const { return m_textDocument; }
    void prepareForAsyncUse();
    void recreateTextDocument();
    AssistReason reason() const;
    bool isBaseObject() const { return m_isBaseObject; }

private:
    QTextDocument *m_textDocument;
    QTextCursor m_cursor;
    bool m_isAsync;
    int m_position;
    int m_anchor;
    Utils::FilePath m_filePath;
    AssistReason m_reason;
    QString m_text;
    QList<int> m_userStates;
    bool m_isBaseObject = true;
};

} // namespace TextEditor
