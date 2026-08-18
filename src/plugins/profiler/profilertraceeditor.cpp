// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertraceeditor.h"

#include "qmlprofilerconstants.h"
#include "profilertr.h"
#include "profilertracebackend.h"
#include "profilertracedocument.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/minisplitter.h>

#include <tracing/rangedetailswidget.h>

#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/widgets.h>

#include <QHBoxLayout>
#include <QTabWidget>

using namespace Core;
using namespace Utils;

namespace Profiler::Internal {

class ProfilerTraceWidget : public MiniSplitter
{
public:
    explicit ProfilerTraceWidget(ProfilerTraceDocument *document)
        : MiniSplitter(Qt::Horizontal)
        , m_tabs(new QTabWidget)
    {
        m_tabs->setDocumentMode(true);
        m_tabs->setTabPosition(QTabWidget::North);
        for (ProfilerTraceBackend *backend : document->backends()) {
            const QWidgetList views = backend->views(m_tabs);
            for (QWidget *view : views)
                m_tabs->addTab(view, view->windowTitle());
        }

        addWidget(m_tabs);
        addWidget(document->rangeDetails());
        setStretchFactor(0, 1);
        setStretchFactor(1, 0);
        setObjectName("ProfilerTraceWidget");

        // Loading and saving run in the background; block interaction with
        // half-filled models meanwhile, as the perspective used to.
        connect(document, &ProfilerTraceDocument::busyChanged,
                this, [this](bool busy) { setEnabled(!busy); });
    }

private:
    QTabWidget *m_tabs = nullptr;
};

class ProfilerTraceEditorPrivate
{
public:
    std::unique_ptr<ProfilerTraceDocument> document;
    ProfilerTraceWidget *widget = nullptr;
    StyledBar *toolBar = nullptr;
    QHBoxLayout *toolBarLayout = nullptr;
};

ProfilerTraceEditor::ProfilerTraceEditor(std::unique_ptr<ProfilerTraceDocument> document)
    : d(new ProfilerTraceEditorPrivate)
{
    d->document = std::move(document);
    d->widget = new ProfilerTraceWidget(d->document.get());
    setWidget(d->widget);
    setContext(Context(Constants::C_PROFILER_TRACE_EDITOR, Core::Constants::C_EDITORMANAGER));
    // One trace, one editor: the views are stateful and the details panel is
    // shared between them, so a duplicate could not show the same trace twice.
    setDuplicateSupported(false);

    d->toolBar = new StyledBar;
    d->toolBarLayout = new QHBoxLayout(d->toolBar);
    using namespace StyleHelper::SpacingTokens;
    d->toolBarLayout->setContentsMargins(PaddingHS, 0, PaddingHS, 0);
    d->toolBarLayout->setSpacing(PrimitiveS);
    for (ProfilerTraceBackend *backend : d->document->backends()) {
        const QList<QWidget *> widgets = backend->toolBarWidgets();
        for (QWidget *widget : widgets)
            d->toolBarLayout->addWidget(widget);
    }
    d->toolBarLayout->addStretch();
}

ProfilerTraceEditor::~ProfilerTraceEditor()
{
    delete d->toolBar;
    delete d->widget;
    delete d;
}

IDocument *ProfilerTraceEditor::document() const
{
    return d->document.get();
}

QWidget *ProfilerTraceEditor::toolBar()
{
    return d->toolBar;
}

// Factories

class ProfilerTraceEditorFactory final : public IEditorFactory
{
public:
    ProfilerTraceEditorFactory(Id id, const QString &displayName, TraceFormat format,
                               const QStringList &mimeTypes)
    {
        setId(id);
        setDisplayName(displayName);
        setMimeTypes(mimeTypes);
        setEditorCreator([id, format] {
            return new ProfilerTraceEditor(std::make_unique<ProfilerTraceDocument>(id, format));
        });
    }
};

static QList<ProfilerTraceEditorFactory *> s_factories;

void setupProfilerTraceEditors()
{
    QTC_ASSERT(s_factories.isEmpty(), return);
    s_factories
        << new ProfilerTraceEditorFactory(Constants::QML_TRACE_EDITOR_ID,
                                          Tr::tr("QML Trace Editor"), TraceFormat::Qml,
                                          {"application/x-qmlprofiler-trace"})
        << new ProfilerTraceEditorFactory(Constants::PERF_TRACE_EDITOR_ID,
                                          Tr::tr("Perf Trace Editor"), TraceFormat::Perf,
                                          {"application/x-perfprofiler-trace"})
        // A CTF or sampler trace is a directory, or plain JSON, so neither has a
        // mime type of its own to be offered for. Both are opened by id.
        << new ProfilerTraceEditorFactory(Constants::CTF_TRACE_EDITOR_ID,
                                          Tr::tr("Chrome Trace Format Editor"), TraceFormat::Ctf, {})
        << new ProfilerTraceEditorFactory(Constants::SAMPLER_TRACE_EDITOR_ID,
                                          Tr::tr("Sampler Trace Editor"), TraceFormat::Sampler, {})
        << new ProfilerTraceEditorFactory(Constants::COMBINED_TRACE_EDITOR_ID,
                                          Tr::tr("Combined Trace Editor"), TraceFormat::Combined, {});
}

void destroyProfilerTraceEditors()
{
    qDeleteAll(s_factories);
    s_factories.clear();
}

} // namespace Profiler::Internal
