// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsprojectsettings.h"

#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

#include <QObject>
#include <QTimer>

namespace Core { class IDocument; }
namespace Tasking { class TaskTree; }
namespace TextEditor { class TextEditorWidget; }

namespace ClangTools {
namespace Internal {

struct AnalyzeOutputData;
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

    void onDone(const AnalyzeOutputData &output);
    void finalize();

    bool isSuppressed(const Diagnostic &diagnostic) const;

    QTimer m_runTimer;
    Core::IDocument *m_document = nullptr;
    Utils::TemporaryDirectory m_temporaryDir;
    QList<DiagnosticMark *> m_marks;
    FileInfo m_fileInfo;
    QMetaObject::Connection m_projectSettingsUpdate;
    QList<QPointer<TextEditor::TextEditorWidget>> m_editorsWithMarkers;
    SuppressedDiagnosticsList m_suppressed;
    Utils::FilePath m_lastProjectDirectory;
    std::unique_ptr<Tasking::TaskTree> m_taskTree;
};

} // namespace Internal
} // namespace ClangTools
