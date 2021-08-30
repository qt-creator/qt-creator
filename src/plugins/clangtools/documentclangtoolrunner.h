/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

    const CppEditor::ClangDiagnosticConfig getDiagnosticConfig(ProjectExplorer::Project *project);
    template<class T>
    ClangToolRunner *createRunner(const CppEditor::ClangDiagnosticConfig &config,
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
