// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtextmark.h"

#include "clangcodemodeltr.h"
#include "clangconstants.h"
#include "clangdclient.h"
#include "clangdiagnostictooltipwidget.h"
#include "clangutils.h"

#include <coreplugin/icore.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cppcodemodelsettings.h>

#include <projectexplorer/task.h>

#include <utils/fadingindicator.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QLayout>
#include <QMainWindow>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>

using namespace CppEditor;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

namespace {

Project *projectForCurrentEditor()
{
    const FilePath filePath = currentCppEditorDocumentFilePath();
    if (filePath.isEmpty())
        return nullptr;

    if (ProjectPart::ConstPtr projectPart = projectPartForFile(filePath))
        return projectForProjectPart(*projectPart);

    return nullptr;
}

void disableDiagnosticInConfig(ClangDiagnosticConfig &config, const ClangDiagnostic &diagnostic)
{
    config.setClangOptions(config.clangOptions() + QStringList(diagnostic.disableOption));
}

ClangDiagnosticConfig diagnosticConfig()
{
    Project *project = projectForCurrentEditor();
    QTC_ASSERT(project, return {});
    return warningsConfigForProject(project);
}

static bool isDiagnosticConfigChangable(Project *project)
{
    if (!project)
        return false;
    if (diagnosticConfig().useBuildSystemWarnings())
        return false;
    return true;
}

void disableDiagnosticInCurrentProjectConfig(const ClangDiagnostic &diagnostic)
{
    Project *project = projectForCurrentEditor();
    QTC_ASSERT(project, return );

    // Get config
    ClangDiagnosticConfig config = diagnosticConfig();
    ClangDiagnosticConfigsModel configsModel = CppEditor::diagnosticConfigsModel();

    // Create copy if needed
    if (config.isReadOnly()) {
        const QString name = Tr::tr("Project: %1 (based on %2)")
                                 .arg(project->displayName(), config.displayName());
        config = ClangDiagnosticConfigsModel::createCustomConfig(config, name);
    }

    // Modify diagnostic config
    disableDiagnosticInConfig(config, diagnostic);
    configsModel.appendOrUpdate(config);

    // Set global settings
    ClangdSettings::setCustomDiagnosticConfigs(configsModel.customConfigs());

    // Set project settings
    ClangdProjectSettings projectSettings(project);
    if (projectSettings.useGlobalSettings())
        projectSettings.setUseGlobalSettings(false);
    projectSettings.setDiagnosticConfigId(config.id());

    // Notify the user about changed project specific settings
    const QString text
        = Tr::tr("Changes applied to diagnostic configuration \"%1\".").arg(config.displayName());
    FadingIndicator::showText(Core::ICore::mainWindow(),
                              text,
                              FadingIndicator::SmallText);
}

ClangDiagnostic::Severity convertSeverity(DiagnosticSeverity src)
{
    if (src == DiagnosticSeverity::Error)
        return ClangDiagnostic::Severity::Error;
    if (src == DiagnosticSeverity::Warning)
        return ClangDiagnostic::Severity::Warning;
    return ClangDiagnostic::Severity::Note;
}

ClangSourceRange convertRange(const FilePath &filePath, const Range &src)
{
    const Utils::Link start(filePath, src.start().line() + 1, src.start().character());
    const Utils::Link end(filePath, src.end().line() + 1, src.end().character());
    return ClangSourceRange(start, end);
}

ClangDiagnostic convertDiagnostic(const ClangdDiagnostic &src,
                                  const FilePath &filePath,
                                  const DocumentUri::PathMapper &mapper)
{
    ClangDiagnostic target;
    target.location = convertRange(filePath, src.range()).start;
    const QStringList messages = src.message().split("\n\n", Qt::SkipEmptyParts);
    if (!messages.isEmpty())
        target.text = messages.first();
    for (int i = 1; i < messages.size(); ++i) {
        QString auxMessage = messages.at(i);
        auxMessage.replace('\n', ' ');

        // TODO: Taken from ClangParser; consolidate
        static const QRegularExpression msgRegex(
                    "^(<command line>|([A-Za-z]:)?[^:]+\\.[^:]+)"
                    "(:(\\d+):(\\d+)|\\((\\d+)\\) *): +(fatal +)?(error|warning|note): (.*)$");

        ClangDiagnostic aux;
        if (const QRegularExpressionMatch match = msgRegex.match(auxMessage); match.hasMatch()) {
            bool ok = false;
            int line = match.captured(4).toInt(&ok);
            int column = match.captured(5).toInt();
            if (!ok) {
                line = match.captured(6).toInt(&ok);
                column = 0;
            }
            FilePath auxFilePath = mapper(FilePath::fromUserInput(match.captured(1)));
            if (auxFilePath.isRelativePath() && auxFilePath.fileName() == filePath.fileName())
                auxFilePath = filePath;
            aux.location = {auxFilePath, line, column - 1};
            aux.text = match.captured(9);
            const QString type = match.captured(8);
            if (type == "fatal")
                aux.severity = ClangDiagnostic::Severity::Fatal;
            else if (type == "error")
                aux.severity = ClangDiagnostic::Severity::Error;
            else if (type == "warning")
                aux.severity = ClangDiagnostic::Severity::Warning;
            else if (type == "note")
                aux.severity = ClangDiagnostic::Severity::Note;
        } else {
            aux.text = auxMessage;
        }
        target.children << aux;
    }
    target.category = src.category();
    if (src.severity())
        target.severity = convertSeverity(*src.severity());
    const Diagnostic::Code code = src.code().value_or(Diagnostic::Code());
    const QString * const codeString = std::get_if<QString>(&code);
    if (codeString && codeString->startsWith("-W")) {
        target.enableOption = *codeString;
        target.disableOption = "-Wno-" + codeString->mid(2);
    }
    for (const CodeAction &codeAction : src.codeActions().value_or(QList<CodeAction>())) {
        const std::optional<WorkspaceEdit> edit = codeAction.edit();
        if (!edit)
            continue;
        const std::optional<WorkspaceEdit::Changes> changes = edit->changes();
        if (!changes)
            continue;
        ClangDiagnostic fixItDiag;
        fixItDiag.text = codeAction.title();
        for (auto it = changes->cbegin(); it != changes->cend(); ++it) {
            for (const TextEdit &textEdit : it.value()) {
                fixItDiag.fixIts << ClangFixIt(textEdit.newText(),
                                               convertRange(it.key().toFilePath(mapper),
                                                            textEdit.range()));
            }
        }
        target.children << fixItDiag;
    }
    return target;
}

Task createTask(const ClangDiagnostic &diagnostic)
{
    Task::TaskType taskType = Task::TaskType::Unknown;
    QIcon icon;

    switch (diagnostic.severity) {
    case ClangDiagnostic::Severity::Fatal:
    case ClangDiagnostic::Severity::Error:
        taskType = Task::TaskType::Error;
        icon = ::Utils::Icons::CODEMODEL_ERROR.icon();
        break;
    case ClangDiagnostic::Severity::Warning:
        taskType = Task::TaskType::Warning;
        icon = ::Utils::Icons::CODEMODEL_WARNING.icon();
        break;
    default:
        break;
    }

    return Task(taskType,
                diagnosticCategoryPrefixRemoved(diagnostic.text),
                diagnostic.location.targetFilePath,
                diagnostic.location.targetLine,
                Constants::TASK_CATEGORY_DIAGNOSTICS,
                icon,
                Task::NoOptions);
}

} // anonymous namespace

ClangdTextMark::ClangdTextMark(TextEditor::TextDocument *doc,
                               const Diagnostic &diagnostic,
                               bool isProjectFile,
                               ClangdClient *client)
    : TextEditor::TextMark(doc,
                           int(diagnostic.range().start().line() + 1),
                           {client->name(), client->id()})
    , m_lspDiagnostic(diagnostic)
    , m_diagnostic(
          convertDiagnostic(ClangdDiagnostic(diagnostic), doc->filePath(), client->hostPathMapper()))
    , m_client(client)
{
    setSettingsPage(CppEditor::Constants::CPP_CLANGD_SETTINGS_ID);

    const bool isError = diagnostic.severity()
            && *diagnostic.severity() == DiagnosticSeverity::Error;
    setDefaultToolTip(isError ? Tr::tr("Code Model Error") : Tr::tr("Code Model Warning"));
    setPriority(isError ? TextEditor::TextMark::HighPriority
                        : TextEditor::TextMark::NormalPriority);
    setIcon(isError ? Icons::CODEMODEL_ERROR.icon() : Icons::CODEMODEL_WARNING.icon());
    if (isProjectFile) {
        setLineAnnotation(diagnostic.message());
        setColor(isError ? Theme::CodeModel_Error_TextMarkColor
                         : Theme::CodeModel_Warning_TextMarkColor);
        client->addTask(createTask(m_diagnostic));
    }

    setActionsProvider([diag = m_diagnostic] {
        // Copy to clipboard action
        QList<QAction *> actions;
        QAction *action = new QAction();
        action->setIcon(Icon::fromTheme("edit-copy"));
        action->setToolTip(Tr::tr("Copy to Clipboard", "Clang Code Model Marks"));
        QObject::connect(action, &QAction::triggered, [diag] {
            const QString text = ClangDiagnosticWidget::createText({diag},
                                                                   ClangDiagnosticWidget::InfoBar);
            setClipboardAndSelection(text);
        });
        actions << action;

        // Remove diagnostic warning action
        if (!diag.disableOption.isEmpty()) {
            if (Project * const project = projectForCurrentEditor();
                project && isDiagnosticConfigChangable(project)) {
                action = new QAction();
                action->setIcon(Icons::BROKEN.icon());
                action->setToolTip(Tr::tr("Disable Diagnostic in Current Project"));
                QObject::connect(action, &QAction::triggered, [diag] {
                    disableDiagnosticInCurrentProjectConfig(diag);
                });
                actions << action;
            }
        }
        return actions;
    });
}

bool ClangdTextMark::addToolTipContent(QLayout *target) const
{
    const auto canApplyFixIt = [c = m_client, diag = m_lspDiagnostic, fp = filePath()] {
        return QTC_GUARD(c) && c->reachable() && c->hasDiagnostic(fp, diag);
    };
    const QString clientName = QTC_GUARD(m_client) ? m_client->name() : "clangd [unknown]";
    target->addWidget(ClangDiagnosticWidget::createWidget({m_diagnostic},
                                                          ClangDiagnosticWidget::ToolTip,
                                                          canApplyFixIt,
                                                          clientName));
    return true;
}

} // namespace Internal
} // namespace ClangCodeModel
