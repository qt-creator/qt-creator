// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsprojectsettings.h"

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <QObject>
#include <QTimer>

namespace Core { class IDocument; }
namespace CppEditor { class ClangDiagnosticConfig; }
namespace TextEditor { class TextEditorWidget; }

namespace ClangTools {

namespace Internal {

class ClangToolRunner;
class DiagnosticMark;

class DocumentClangToolRunner : public QObject
{
    Q_OBJECT
public:
    DocumentClangToolRunner(Core::IDocument *doc);
    ~DocumentClangToolRunner();

    Utils::FilePath filePath() const;
    Diagnostics diagnosticsAtLine(int lineNumber) const;

private:
    void scheduleRun();
    void run();
    void runNext();

    void onSuccess();
    void onFailure(const QString &errorMessage, const QString &errorDetails);

    void finalize();

    void cancel();

    bool isSuppressed(const Diagnostic &diagnostic) const;

    ClangToolRunner *createRunner(CppEditor::ClangToolType tool,
                                  const CppEditor::ClangDiagnosticConfig &config,
                                  const Utils::Environment &env);

    QTimer m_runTimer;
    Core::IDocument *m_document = nullptr;
    Utils::TemporaryDirectory m_temporaryDir;
    std::unique_ptr<ClangToolRunner> m_currentRunner;
    QList<std::function<ClangToolRunner *()>> m_runnerCreators;
    QList<DiagnosticMark *> m_marks;
    FileInfo m_fileInfo;
    QMetaObject::Connection m_projectSettingsUpdate;
    QList<QPointer<TextEditor::TextEditorWidget>> m_editorsWithMarkers;
    SuppressedDiagnosticsList m_suppressed;
    Utils::FilePath m_lastProjectDirectory;
};

} // namespace Internal
} // namespace ClangTools
