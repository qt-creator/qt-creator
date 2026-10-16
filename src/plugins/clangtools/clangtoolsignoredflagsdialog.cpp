// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsignoredflagsdialog.h"

#include "clangtoolssettings.h"
#include "clangtoolstr.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/project.h>

#include <utils/infobar.h>

#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools::Internal {

static void appendIgnoredFlags(RunSettings &settings, const QStringList &flags)
{
    QStringList present = settings.ignoredCompilerFlagPatterns();
    for (const QString &flag : flags) {
        if (!present.contains(flag))
            present << flag;
    }
    settings.ignoredCompilerFlags.setValue(present.join('\n'));
}

static void addToIgnoredFlags(Project *project, bool isProjectScope, const QStringList &flags)
{
    if (isProjectScope) {
        if (!project)
            return;
        const auto settings = clangToolsProjectSettings(project);
        appendIgnoredFlags(*settings, flags);
        return;
    }
    ClangToolsSettings *settings = ClangToolsSettings::instance();
    appendIgnoredFlags(*settings, flags);
    settings->writeSettings();
}

static QSet<QString> &offeredFlags()
{
    static QSet<QString> offered;
    return offered;
}

// Flags are offered once per settings scope: the global one or the one of a project
static QString offerScope(const Project *project, bool isProjectScope)
{
    return isProjectScope ? project->projectFilePath().toUrlishString() : QString();
}

static QStringList flagsNotOffered(const QString &scope, const QStringList &flags)
{
    QStringList result;
    for (const QString &flag : flags) {
        if (!offeredFlags().contains(scope + '\n' + flag))
            result << flag;
    }
    return result;
}

static void markOffered(const QString &scope, const QStringList &flags)
{
    for (const QString &flag : flags)
        offeredFlags().insert(scope + '\n' + flag);
}

static bool isProjectScopeFor(Project *project)
{
    return project && !clangToolsProjectSettings(project)->useGlobalSettings();
}

// The same as the dialog below, but for analyses that nobody started explicitly: the offer is
// shown in the info bar of the document.
void offerToIgnoreCompilerFlags(Core::IDocument *document, Project *project,
                                const QStringList &flags)
{
    // Flags that cannot be shown now stay unoffered, so that a later analysis offers them again
    const Id infoId("ClangTools.UnknownCompilerFlags");
    if (!document->infoBar()->canInfoBeAdded(infoId))
        return;
    const bool isProjectScope = isProjectScopeFor(project);
    const QString scope = offerScope(project, isProjectScope);
    const QStringList newFlags = flagsNotOffered(scope, flags);
    if (newFlags.isEmpty())
        return;

    // The offer stays valid until it is answered: an info bar that goes away with its
    // document was not seen.
    InfoBarEntry info(infoId,
                      Tr::tr("Clang does not know these compiler flags from the build system: "
                             "%1. Ignore them in the analysis?").arg(newFlags.join(' ')));
    info.addCustomButton(
        Tr::tr("Add to Ignored Flags"),
        [project = QPointer(project), isProjectScope, scope, newFlags] {
            addToIgnoredFlags(project, isProjectScope, newFlags);
            markOffered(scope, newFlags);
        },
        isProjectScope ? Tr::tr("Adds the flags to the project's Clang Tools settings.")
                       : Tr::tr("Adds the flags to the global Clang Tools settings."),
        InfoBarEntry::ButtonAction::Hide);
    info.setCancelButtonInfo([scope, newFlags] { markOffered(scope, newFlags); });
    document->infoBar()->addInfo(info);
}

// Shows a dialog that offers to add the flags to the "Ignored compiler flags" setting of the
// project (or the global one, if the project uses it) and to copy them to the clipboard.
// A flag is offered only once per session and settings scope.
void offerToIgnoreCompilerFlags(Project *project, const QStringList &flags)
{
    const bool isProjectScope = isProjectScopeFor(project);
    const QString scope = offerScope(project, isProjectScope);
    const QStringList newFlags = flagsNotOffered(scope, flags);
    if (newFlags.isEmpty())
        return;
    markOffered(scope, newFlags);

    auto dialog = new QDialog(Core::ICore::dialogParent());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(Tr::tr("Unknown Compiler Flags"));

    const QString scopeText = isProjectScope
        ? Tr::tr("They will be added to \"Ignored compiler flags\" in the project's Clang Tools "
                 "settings.")
        : Tr::tr("They will be added to \"Ignored compiler flags\" in the global Clang Tools "
                 "settings.");
    const QString text
        = Tr::tr("Clang reported the following compiler flags from the build system as unknown. "
                 "Clang-Tidy and Clazy may not analyze the affected files completely.")
          + "\n\n" + Tr::tr("Ignore these flags in the analysis?") + ' ' + scopeText;
    auto label = new QLabel(text);
    label->setWordWrap(true);

    auto flagsEdit = new QPlainTextEdit;
    flagsEdit->setReadOnly(true);
    flagsEdit->setPlainText(newFlags.join('\n'));
    const QFontMetrics metrics = flagsEdit->fontMetrics();
    flagsEdit->setMinimumSize(metrics.averageCharWidth() * 40, metrics.height() * 6);

    auto buttons = new QDialogButtonBox;
    QPushButton *addButton
        = buttons->addButton(Tr::tr("Add to Ignored Flags"), QDialogButtonBox::AcceptRole);
    QPushButton *copyButton
        = buttons->addButton(Tr::tr("Copy Flags"), QDialogButtonBox::ActionRole);
    buttons->addButton(Tr::tr("Close"), QDialogButtonBox::RejectRole);
    addButton->setDefault(true);

    auto layout = new QVBoxLayout(dialog);
    layout->addWidget(label);
    layout->addWidget(flagsEdit);
    layout->addWidget(buttons);

    QObject::connect(copyButton, &QPushButton::clicked, dialog, [newFlags] {
        QGuiApplication::clipboard()->setText(newFlags.join('\n'));
    });
    QObject::connect(
        buttons,
        &QDialogButtonBox::accepted,
        dialog,
        [dialog, project = QPointer(project), isProjectScope, newFlags] {
            addToIgnoredFlags(project, isProjectScope, newFlags);
            dialog->accept();
        });
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    dialog->show();
}

} // namespace ClangTools::Internal
