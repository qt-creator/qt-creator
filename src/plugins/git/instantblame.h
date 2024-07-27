// Copyright (C) 2023 Andre Hartmann (aha_1980@gmx.de)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitclient.h"

#include <texteditor/textmark.h>

#include <utils/filepath.h>

QT_BEGIN_NAMESPACE
class QLayout;
class QTextCodec;
class QTimer;
QT_END_NAMESPACE

namespace Git::Internal {

class CommitInfo {
public:
    QString sha1;
    QString shortAuthor;
    QString author;
    QString authorMail;
    QDateTime authorTime;
    QString summary;
    Utils::FilePath filePath; ///< absolute file path for current file
    QString originalFileName; ///< relative file path from project root for the original file
    int line = -1;            ///< current line number in current file
    int originalLine = -1;    ///< original line number in the original file
};

class BlameMark : public TextEditor::TextMark
{
public:
    BlameMark(const Utils::FilePath &fileName, int lineNumber, const CommitInfo &info);
    bool addToolTipContent(QLayout *target) const;
    QString toolTipText(const CommitInfo &info) const;

private:
    const CommitInfo m_info;
};

class InstantBlame : public QObject
{
    Q_OBJECT

public:
    InstantBlame();

    void setup();
    void once();
    void force();
    void stop();
    void perform();

private:
    bool refreshWorkingDirectory(const Utils::FilePath &workingDirectory);
    void slotDocumentChanged();

    Utils::FilePath m_workingDirectory;
    QTextCodec *m_codec = nullptr;
    Author m_author;
    int m_lastVisitedEditorLine = -1;
    Core::IDocument *m_document = nullptr;
    bool m_modified = false;
    QTimer *m_cursorPositionChangedTimer = nullptr;
    std::unique_ptr<BlameMark> m_blameMark;
    QMetaObject::Connection m_blameCursorPosConn;
    QMetaObject::Connection m_documentChangedConn;
};

} // Git::Internal
