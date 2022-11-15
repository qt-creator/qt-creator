// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "pluginerrorview.h"

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

        using namespace Utils::Layouting;

        Form {
            QCoreApplication::translate("ExtensionSystem::Internal::PluginErrorView",
                                        "State:"), state, br,
            QCoreApplication::translate("ExtensionSystem::Internal::PluginErrorView",
                                        "Error message:"), errorString
        }.attachTo(q, WithoutMargins);
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
        text = tr("Invalid");
        tooltip = tr("Description file found, but error on read.");
        break;
    case PluginSpec::Read:
        text = tr("Read");
        tooltip = tr("Description successfully read.");
        break;
    case PluginSpec::Resolved:
        text = tr("Resolved");
        tooltip = tr("Dependencies are successfully resolved.");
        break;
    case PluginSpec::Loaded:
        text = tr("Loaded");
        tooltip = tr("Library is loaded.");
        break;
    case PluginSpec::Initialized:
        text = tr("Initialized");
        tooltip = tr("Plugin's initialization function succeeded.");
        break;
    case PluginSpec::Running:
        text = tr("Running");
        tooltip = tr("Plugin successfully loaded and running.");
        break;
    case PluginSpec::Stopped:
        text = tr("Stopped");
        tooltip = tr("Plugin was shut down.");
        break;
    case PluginSpec::Deleted:
        text = tr("Deleted");
        tooltip = tr("Plugin ended its life cycle and was deleted.");
        break;
    }

    d->state->setText(text);
    d->state->setToolTip(tooltip);
    d->errorString->setText(spec->errorString());
}
