// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <memory>

namespace Profiler::Internal {

class ProfilerTraceDocument;

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

void setupProfilerTraceEditors();
void destroyProfilerTraceEditors();

} // namespace Profiler::Internal
