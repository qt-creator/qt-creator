// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <memory>

namespace Utils { class FilePath; }

namespace Profiler::Internal {

class ProfilerTraceDocument;
enum class TraceFormat;

// Shows one trace: its backends' views as tabs, with the shared details panel
// beside them.
class ProfilerTraceEditor : public Core::IEditor
{
    Q_OBJECT

public:
    explicit ProfilerTraceEditor(std::unique_ptr<ProfilerTraceDocument> document);
    ~ProfilerTraceEditor() override;

    Core::IDocument *document() const override;
    QWidget *toolBar() override;

private:
    class ProfilerTraceEditorPrivate *d;
};

// Opens `path` as a trace editor, choosing the editor from the file's format.
Core::IEditor *openTraceFile(const Utils::FilePath &path);

// Opens an empty trace document for a profiling run to record into. `uniqueId`
// identifies the run: opening with one that is already showing reuses its
// editor rather than adding another.
ProfilerTraceDocument *openLiveTrace(TraceFormat format, const QString &title,
                                     const QString &uniqueId);

void setupProfilerTraceEditors();
void destroyProfilerTraceEditors();

} // namespace Profiler::Internal
