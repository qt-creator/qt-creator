// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    virtual bool isBaseObject() const { return true; }

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
