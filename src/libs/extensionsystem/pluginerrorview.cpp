// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pluginerrorview.h"

#include "extensionsystemtr.h"
#include "pluginspec.h"

#include <utils/layoutbuilder.h>

#include <QCoreApplication>
#include <QLabel>
#include <QTextEdit>

/*!
    \class ExtensionSystem::PluginErrorView
    \inheaderfile extensionsystem/pluginerrorview.h
    \inmodule QtCreator

    \brief The PluginErrorView class implements a widget that displays the
    state and error message of a PluginSpec.

    Can be used for integration in the application that
    uses the plugin manager.

    \sa ExtensionSystem::PluginView
*/

using namespace ExtensionSystem;

namespace ExtensionSystem::Internal {

class PluginErrorViewPrivate
{
public:
    PluginErrorViewPrivate(PluginErrorView *view)
        : q(view)
        , state(new QLabel(q))
        , errorString(new QTextEdit(q))
    {
        errorString->setTabChangesFocus(true);
        errorString->setReadOnly(true);

        using namespace Layouting;

        Form {
            Tr::tr("State:"), state, br,
            Tr::tr("Error message:"), errorString,
            noMargin,
        }.attachTo(q);
    }

    PluginErrorView *q = nullptr;
    QLabel *state = nullptr;
    QTextEdit *errorString = nullptr;
};

} // namespace ExtensionSystem::Internal

using namespace Internal;

/*!
    Constructs a new error view with given \a parent widget.
*/
PluginErrorView::PluginErrorView(QWidget *parent)
    : QWidget(parent),
      d(new PluginErrorViewPrivate(this))
{
}

PluginErrorView::~PluginErrorView()
{
    delete d;
}

/*!
    Reads the given \a spec and displays its state and
    error information in this PluginErrorView.
*/
void PluginErrorView::update(PluginSpec *spec)
{
    QString text;
    QString tooltip;
    switch (spec->state()) {
    case PluginSpec::Invalid:
        text = Tr::tr("Invalid");
        tooltip = Tr::tr("Description file found, but error on read.");
        break;
    case PluginSpec::Read:
        text = Tr::tr("Read");
        tooltip = Tr::tr("Description successfully read.");
        break;
    case PluginSpec::Resolved:
        text = Tr::tr("Resolved");
        tooltip = Tr::tr("Dependencies are successfully resolved.");
        break;
    case PluginSpec::Loaded:
        text = Tr::tr("Loaded");
        tooltip = Tr::tr("Library is loaded.");
        break;
    case PluginSpec::Initialized:
        text = Tr::tr("Initialized");
        tooltip = Tr::tr("Plugin's initialization function succeeded.");
        break;
    case PluginSpec::Running:
        text = Tr::tr("Running");
        tooltip = Tr::tr("Plugin successfully loaded and running.");
        break;
    case PluginSpec::Stopped:
        text = Tr::tr("Stopped");
        tooltip = Tr::tr("Plugin was shut down.");
        break;
    case PluginSpec::Deleted:
        text = Tr::tr("Deleted");
        tooltip = Tr::tr("Plugin ended its life cycle and was deleted.");
        break;
    }

    d->state->setText(text);
    d->state->setToolTip(tooltip);
    d->errorString->setText(spec->errorString());
}
