// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findincurrentfile.h"

#include "basefilefind.h"
#include "textdocument.h"
#include "texteditortr.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcsettings.h>

#include <QPointer>
#include <QTextCodec>

using namespace Utils;

namespace TextEditor::Internal {

class FindInCurrentFile final : public BaseFileFind
{
public:
    FindInCurrentFile();

private:
    QString id() const final;
    QString displayName() const final;
    bool isEnabled() const final;
    Utils::Store save() const final;
    void restore(const Utils::Store &s) final;

    QString label() const final;
    QString toolTip() const final;

    FileContainerProvider fileContainerProvider() const final;
    void handleFileChange(Core::IEditor *editor);

    QPointer<Core::IDocument> m_currentDocument;

    Utils::FindFlags supportedFindFlags() const override
    {
        return FindCaseSensitively | FindRegularExpression | FindWholeWords;
    }

    // deprecated
    QByteArray settingsKey() const final;
};

FindInCurrentFile::FindInCurrentFile()
{
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &FindInCurrentFile::handleFileChange);
    handleFileChange(Core::EditorManager::currentEditor());
}

QString FindInCurrentFile::id() const
{
    return QLatin1String("Current File");
}

QString FindInCurrentFile::displayName() const
{
    return Tr::tr("Current File");
}

FileContainerProvider FindInCurrentFile::fileContainerProvider() const
{
    return [fileName = m_currentDocument->filePath()] {
        const QMap<FilePath, QByteArray> encodings = TextDocument::openedTextDocumentEncodings();
        QByteArray codec = encodings.value(fileName);
        if (codec.isEmpty())
            codec = Core::EditorManager::defaultTextCodecName();
        return FileListContainer({fileName}, {QTextCodec::codecForName(codec)});
    };
}

QString FindInCurrentFile::label() const
{
    return Tr::tr("File \"%1\":").arg(m_currentDocument->filePath().fileName());
}

QString FindInCurrentFile::toolTip() const
{
    // %2 is filled by BaseFileFind::runNewSearch
    return Tr::tr("File path: %1\n%2").arg(m_currentDocument->filePath().toUserOutput());
}

bool FindInCurrentFile::isEnabled() const
{
    return m_currentDocument && !m_currentDocument->filePath().isEmpty();
}

void FindInCurrentFile::handleFileChange(Core::IEditor *editor)
{
    if (!editor) {
        m_currentDocument = nullptr;
        emit enabledChanged(isEnabled());
    } else {
        Core::IDocument *document = editor->document();
        if (document != m_currentDocument) {
            m_currentDocument = document;
            emit enabledChanged(isEnabled());
        }
    }
}

const char kDefaultInclusion[] = "*";
const char kDefaultExclusion[] = "";

Store FindInCurrentFile::save() const
{
    Store s;
    writeCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
    return s;
}

void FindInCurrentFile::restore(const Store &s)
{
    readCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
}

QByteArray FindInCurrentFile::settingsKey() const
{
    return "FindInCurrentFile";
}

void setupFindInCurrentFile()
{
    static FindInCurrentFile theFindInCurrentFile;
}

} // TextEditor::Internal
