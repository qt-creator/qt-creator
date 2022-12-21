// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pyside.h"

#include "pipsupport.h"
#include "pythonconstants.h"
#include "pythonplugin.h"
#include "pythonproject.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/icore.h>

#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QRegularExpression>
#include <QTextCursor>

using namespace Utils;
using namespace ProjectExplorer;

namespace Python::Internal {

const char installPySideInfoBarId[] = "Python::InstallPySide";

PySideInstaller *PySideInstaller::instance()
{
    static PySideInstaller *instance = new PySideInstaller; // FIXME: Leaks.
    return instance;
}

void PySideInstaller::checkPySideInstallation(const FilePath &python,
                                              TextEditor::TextDocument *document)
{
    document->infoBar()->removeInfo(installPySideInfoBarId);
    const QString pySide = importedPySide(document->plainText());
    if (pySide == "PySide2" || pySide == "PySide6")
        instance()->runPySideChecker(python, pySide, document);
}

bool PySideInstaller::missingPySideInstallation(const FilePath &pythonPath,
                                                const QString &pySide)
{
    QTC_ASSERT(!pySide.isEmpty(), return false);
    static QMap<FilePath, QSet<QString>> pythonWithPyside;
    if (pythonWithPyside[pythonPath].contains(pySide))
        return false;

    QtcProcess pythonProcess;
    pythonProcess.setCommand({pythonPath, {"-c", "import " + pySide}});
    pythonProcess.runBlocking();
    const bool missing = pythonProcess.result() != ProcessResult::FinishedWithSuccess;
    if (!missing)
        pythonWithPyside[pythonPath].insert(pySide);
    return missing;
}

QString PySideInstaller::importedPySide(const QString &text)
{
    static QRegularExpression importScanner("^\\s*(import|from)\\s+(PySide\\d)",
                                            QRegularExpression::MultilineOption);
    const QRegularExpressionMatch match = importScanner.match(text);
    return match.captured(2);
}

PySideInstaller::PySideInstaller()
    : QObject(PythonPlugin::instance())
{}

void PySideInstaller::installPyside(const FilePath &python,
                                    const QString &pySide,
                                    TextEditor::TextDocument *document)
{
    document->infoBar()->removeInfo(installPySideInfoBarId);

    auto install = new PipInstallTask(python);
    connect(install, &PipInstallTask::finished, install, &QObject::deleteLater);
    connect(install, &PipInstallTask::finished, this, [=](bool success){
        if (success)
            emit pySideInstalled(python, pySide);
    });
    install->setPackage(PipPackage(pySide));
    install->run();
}

void PySideInstaller::changeInterpreter(const QString &interpreterId,
                                        RunConfiguration *runConfig)
{
    if (runConfig) {
        if (auto aspect = runConfig->aspect<InterpreterAspect>())
            aspect->setCurrentInterpreter(PythonSettings::interpreter(interpreterId));
    }
}

void PySideInstaller::handlePySideMissing(const FilePath &python,
                                          const QString &pySide,
                                          TextEditor::TextDocument *document)
{
    if (!document || !document->infoBar()->canInfoBeAdded(installPySideInfoBarId))
        return;
    const QString message = Tr::tr("%1 installation missing for %2 (%3)")
                                .arg(pySide, pythonName(python), python.toUserOutput());
    InfoBarEntry info(installPySideInfoBarId, message, InfoBarEntry::GlobalSuppression::Enabled);
    auto installCallback = [=]() { installPyside(python, pySide, document); };
    const QString installTooltip = Tr::tr("Install %1 for %2 using pip package installer.")
                                       .arg(pySide, python.toUserOutput());
    info.addCustomButton(Tr::tr("Install"), installCallback, installTooltip);

    if (PythonProject *project = pythonProjectForFile(document->filePath())) {
        if (ProjectExplorer::Target *target = project->activeTarget()) {
            auto runConfiguration = target->activeRunConfiguration();
            if (runConfiguration->id() == Constants::C_PYTHONRUNCONFIGURATION_ID) {
                const QList<InfoBarEntry::ComboInfo> interpreters = Utils::transform(
                    PythonSettings::interpreters(), [](const Interpreter &interpreter) {
                        return InfoBarEntry::ComboInfo{interpreter.name, interpreter.id};
                    });
                auto interpreterChangeCallback =
                    [=, rc = QPointer<RunConfiguration>(runConfiguration)](
                        const InfoBarEntry::ComboInfo &info) {
                        changeInterpreter(info.data.toString(), rc);
                    };

                auto interpreterAspect = runConfiguration->aspect<InterpreterAspect>();
                QTC_ASSERT(interpreterAspect, return);
                const QString id = interpreterAspect->currentInterpreter().id;
                const auto isCurrentInterpreter = Utils::equal(&InfoBarEntry::ComboInfo::data,
                                                               QVariant(id));
                const QString switchTooltip = Tr::tr("Switch the Python interpreter for %1")
                                                  .arg(runConfiguration->displayName());
                info.setComboInfo(interpreters,
                                  interpreterChangeCallback,
                                  switchTooltip,
                                  Utils::indexOf(interpreters, isCurrentInterpreter));
            }
        }
    }
    document->infoBar()->addInfo(info);
}

void PySideInstaller::runPySideChecker(const FilePath &python,
                                       const QString &pySide,
                                       TextEditor::TextDocument *document)
{
    using CheckPySideWatcher = QFutureWatcher<bool>;

    QPointer<CheckPySideWatcher> watcher = new CheckPySideWatcher();

    // cancel and delete watcher after a 10 second timeout
    QTimer::singleShot(10000, this, [watcher]() {
        if (watcher) {
            watcher->cancel();
            watcher->deleteLater();
        }
    });
    connect(watcher,
            &CheckPySideWatcher::resultReadyAt,
            this,
            [=, document = QPointer<TextEditor::TextDocument>(document)]() {
                if (watcher->result())
                    handlePySideMissing(python, pySide, document);
                watcher->deleteLater();
            });
    watcher->setFuture(
        Utils::runAsync(&missingPySideInstallation, python, pySide));
}

} // Python::Internal
