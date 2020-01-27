// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownviewer.h"
#include "markdownviewerconstants.h"
#include "markdownviewerwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFileInfo>
#include <QDesktopServices>
#include <QWidget>
#include <QHBoxLayout>

namespace Markdown {
namespace Internal {

struct MarkdownViewerPrivate
{
    MarkdownViewerWidget *widget;
    QWidget *toolbar;
    QToolButton *toggleEditorVisible;
};

MarkdownViewer::MarkdownViewer()
    : d(new MarkdownViewerPrivate)
{
    ctor();
}

void MarkdownViewer::ctor()
{
    d->widget = new MarkdownViewerWidget;

    setContext(Core::Context(Constants::MARKDOWNVIEWER_ID));
    setWidget(d->widget);

    d->toolbar = new QWidget;
    auto layout = new QHBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();

    d->toggleEditorVisible = new QToolButton;
    d->toggleEditorVisible->setText(tr("Hide Editor"));
    d->toggleEditorVisible->setCheckable(true);
    d->toggleEditorVisible->setChecked(true);
    layout->addWidget(d->toggleEditorVisible);

    d->toolbar->setLayout(layout);

    // connections
    connect(d->toggleEditorVisible, &QToolButton::toggled, d->widget, &MarkdownViewerWidget::setEditorVisible);
    connect(d->widget, &MarkdownViewerWidget::editorVisibleChanged, this, [this](bool editorVisible) {
        d->toggleEditorVisible->setText(editorVisible ? tr("Hide Editor") : tr("Show Editor"));
    });
}

MarkdownViewer::~MarkdownViewer()
{
    delete d->widget;
    delete d->toolbar;
    delete d;
}

Core::IDocument *MarkdownViewer::document() const
{
    return d->widget->document();
}

QWidget *MarkdownViewer::toolBar()
{
    return d->toolbar;
}

} // namespace Internal
} // namespace Markdown
