// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aitaskbutton.h"

#include "coreplugintr.h"
#include "progressmanager/taskprogress.h"

#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QEvent>

using namespace QtTaskTree;
using namespace Utils;

namespace Core {

class AiTaskButtonPrivate
{
public:
    AiTaskButton::RecipeFactory recipe;
    AiRequest::Mode mode = AiRequest::Interactive;
    QString progressTitle;
    Id progressId;
    QString idleToolTip;
    ProgressIndicator *spinner = nullptr;
    QSingleTaskTreeRunner runner;
};

/*!
    \class Core::AiTaskButton
    \inmodule QtCreator

    \brief The AiTaskButton class is a button with the icon of the AI chat that
    runs a task tree in which an AI does something, such as writing a commit
    message.

    The recipe that setRecipe() sets usually collects the context and then
    runs a Utils::AiRequestTask. While it runs, the button shows a spinner, and
    the progress shows in the status bar. Clicking the button again or canceling
    the progress cancels the task tree, and with it the request.

    The button is hidden while no target can run requests of the mode that
    setRequestMode() sets.

    \sa Utils::AiAssistance
*/

/*!
    Creates a button with \a parent.
*/
AiTaskButton::AiTaskButton(QWidget *parent)
    : QToolButton(parent)
    , d(new AiTaskButtonPrivate)
{
    setAutoRaise(true);
    // Stays clickable through the spinner, to cancel.
    d->spinner = new ProgressIndicator(ProgressIndicatorSize::Small, this);
    d->spinner->setObjectName("aiTaskButtonSpinner");
    d->spinner->hide();
    showRunning(false);
    connect(this, &QToolButton::clicked, this, [this] {
        if (isRunning())
            cancel();
        else
            start();
    });
    AiAssistance::onTargetsChanged(this, [this] { updateVisibility(); });
    updateVisibility();
}

/*!
    Ends a running task tree without calling its handlers.
*/
AiTaskButton::~AiTaskButton()
{
    d->runner.reset();
    delete d;
}

/*!
    Sets the function that returns the \a recipe of the task tree that the
    button runs. It is called each time the button starts.
*/
void AiTaskButton::setRecipe(const RecipeFactory &recipe)
{
    d->recipe = recipe;
}

/*!
    Sets the \a mode of the AI request that the recipe runs, which decides
    whether the button is shown.

    The default is Utils::AiRequest::Interactive.
*/
void AiTaskButton::setRequestMode(AiRequest::Mode mode)
{
    d->mode = mode;
    updateVisibility();
}

/*!
    Shows the progress of a running task tree with \a title in the status bar,
    as a task of type \a id.
*/
void AiTaskButton::setProgress(const QString &title, Id id)
{
    d->progressTitle = title;
    d->progressId = id;
}

/*!
    Sets the \a toolTip of the button while it does not run.
*/
void AiTaskButton::setIdleToolTip(const QString &toolTip)
{
    d->idleToolTip = toolTip;
    showRunning(isRunning());
}

/*!
    Returns whether the task tree runs.
*/
bool AiTaskButton::isRunning() const
{
    return d->runner.isRunning();
}

/*!
    Starts the task tree, unless it runs already.
*/
void AiTaskButton::start()
{
    QTC_ASSERT(d->recipe, return);
    if (isRunning())
        return;
    const auto onTreeSetup = [this](QTaskTree &taskTree) {
        if (d->progressTitle.isEmpty())
            return;
        auto progress = new TaskProgress(&taskTree);
        progress->setDisplayName(d->progressTitle);
        progress->setId(d->progressId);
    };
    showRunning(true);
    d->runner.start(d->recipe(), onTreeSetup, [this] { showRunning(false); });
}

/*!
    Cancels the task tree if it runs.
*/
void AiTaskButton::cancel()
{
    d->runner.cancel();
}

void AiTaskButton::showRunning(bool running)
{
    setIcon(running ? QIcon() : Icons::AI_ASSISTANT.icon());
    d->spinner->setVisible(running);
    setToolTip(running ? Tr::tr("Cancel") : d->idleToolTip);
}

bool AiTaskButton::event(QEvent *event)
{
    if (event->type() == QEvent::ParentChange)
        updateVisibility();
    return QToolButton::event(event);
}

void AiTaskButton::updateVisibility()
{
    // Showing a button without a parent would make it a window.
    if (parentWidget())
        setVisible(AiAssistance::hasTargets(d->mode));
}

} // namespace Core
