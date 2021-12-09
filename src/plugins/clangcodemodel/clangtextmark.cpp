/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangtextmark.h"

#include "clangconstants.h"
#include "clangdclient.h"
#include "clangdiagnostictooltipwidget.h"
#include "clangeditordocumentprocessor.h"
#include "clangmodelmanagersupport.h"
#include "clangprojectsettings.h"
#include "clangutils.h"

#include <coreplugin/icore.h>
#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cppcodemodelsettings.h>

#include <utils/fadingindicator.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLayout>
#include <QString>

using namespace CppEditor;
using namespace ClangCodeModel::Internal;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

namespace {

bool isWarningOrNote(ClangBackEnd::DiagnosticSeverity severity)
{
    using ClangBackEnd::DiagnosticSeverity;
    switch (severity) {
        case DiagnosticSeverity::Ignored:
        case DiagnosticSeverity::Note:
        case DiagnosticSeverity::Warning: return true;
        case DiagnosticSeverity::Error:
        case DiagnosticSeverity::Fatal: return false;
    }

    Q_UNREACHABLE();
}

static Id categoryForSeverity(ClangBackEnd::DiagnosticSeverity severity)
{
    return isWarningOrNote(severity) ? Constants::CLANG_WARNING : Constants::CLANG_ERROR;
}

ProjectExplorer::Project *projectForCurrentEditor()
{
    const QString filePath = currentCppEditorDocumentFilePath();
    if (filePath.isEmpty())
        return nullptr;

    if (auto processor = ClangEditorDocumentProcessor::get(filePath)) {
        if (ProjectPart::ConstPtr projectPart = processor->projectPart())
            return projectForProjectPart(*projectPart);
    }

    return nullptr;
}

enum class DiagnosticType { Clang, Tidy, Clazy };
DiagnosticType diagnosticType(const ClangBackEnd::DiagnosticContainer &diagnostic)

{
    if (!diagnostic.disableOption.isEmpty())
        return DiagnosticType::Clang;

    const DiagnosticTextInfo textInfo(diagnostic.text);
    if (DiagnosticTextInfo::isClazyOption(textInfo.option()))
        return DiagnosticType::Clazy;
    return DiagnosticType::Tidy;
}

void disableDiagnosticInConfig(ClangDiagnosticConfig &config,
                               const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    switch (diagnosticType(diagnostic)) {
    case DiagnosticType::Clang:
        config.setClangOptions(config.clangOptions() + QStringList(diagnostic.disableOption));
        break;
    case DiagnosticType::Tidy:
        config.setClangTidyChecks(config.clangTidyChecks() + QString(",-")
                                  + DiagnosticTextInfo(diagnostic.text).option());
        break;
    case DiagnosticType::Clazy: {
        const DiagnosticTextInfo textInfo(diagnostic.text);
        const QString checkName = DiagnosticTextInfo::clazyCheckName(textInfo.option());
        QStringList newChecks = config.clazyChecks().split(',');
        newChecks.removeOne(checkName);
        config.setClazyChecks(newChecks.join(','));
        break;
    }
    }
}

ClangDiagnosticConfig diagnosticConfig(const ClangProjectSettings &projectSettings,
                                       const CppCodeModelSettings &globalSettings)
{
    ProjectExplorer::Project *project = projectForCurrentEditor();
    QTC_ASSERT(project, return {});

    // Get config id
    Id currentConfigId = projectSettings.warningConfigId();
    if (projectSettings.useGlobalConfig())
        currentConfigId = globalSettings.clangDiagnosticConfigId();

    // Get config
    ClangDiagnosticConfigsModel configsModel = CppEditor::diagnosticConfigsModel();
    QTC_ASSERT(configsModel.hasConfigWithId(currentConfigId), return {});
    return configsModel.configWithId(currentConfigId);
}

bool isDiagnosticConfigChangable(ProjectExplorer::Project *project,
                                 const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    if (!project)
        return false;

    ClangProjectSettings &projectSettings = ClangModelManagerSupport::instance()->projectSettings(
        project);
    const CppCodeModelSettings *globalSettings = codeModelSettings();
    const ClangDiagnosticConfig config = diagnosticConfig(projectSettings, *globalSettings);

    if (config.clangTidyMode() == ClangDiagnosticConfig::TidyMode::UseConfigFile
        && diagnosticType(diagnostic) == DiagnosticType::Tidy) {
        return false;
    }
    return true;
}

void disableDiagnosticInCurrentProjectConfig(const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    ProjectExplorer::Project *project = projectForCurrentEditor();
    QTC_ASSERT(project, return );

    // Get settings
    ClangProjectSettings &projectSettings = ClangModelManagerSupport::instance()->projectSettings(
        project);
    CppCodeModelSettings *globalSettings = codeModelSettings();

    // Get config
    ClangDiagnosticConfig config = diagnosticConfig(projectSettings, *globalSettings);
    ClangDiagnosticConfigsModel configsModel = CppEditor::diagnosticConfigsModel();

    // Create copy if needed
    if (config.isReadOnly()) {
        const QString name = QCoreApplication::translate("ClangDiagnosticConfig",
                                                         "Project: %1 (based on %2)")
                                 .arg(project->displayName(), config.displayName());
        config = ClangDiagnosticConfigsModel::createCustomConfig(config, name);
    }

    // Modify diagnostic config
    disableDiagnosticInConfig(config, diagnostic);
    configsModel.appendOrUpdate(config);

    // Set global settings
    globalSettings->setClangCustomDiagnosticConfigs(configsModel.customConfigs());
    globalSettings->toSettings(Core::ICore::settings());

    // Set project settings
    if (projectSettings.useGlobalConfig())
        projectSettings.setUseGlobalConfig(false);
    projectSettings.setWarningConfigId(config.id());
    projectSettings.store();

    // Notify the user about changed project specific settings
    const QString text
        = QCoreApplication::translate("ClangDiagnosticConfig",
                                      "Changes applied in Projects Mode > Clang Code Model");
    FadingIndicator::showText(Core::ICore::mainWindow(),
                              text,
                              FadingIndicator::SmallText);
}

} // anonymous namespace

ClangTextMark::ClangTextMark(const FilePath &fileName,
                             const ClangBackEnd::DiagnosticContainer &diagnostic,
                             const RemovedFromEditorHandler &removedHandler,
                             bool fullVisualization, const ClangDiagnosticManager *diagMgr)
    : TextEditor::TextMark(fileName,
                           int(diagnostic.location.line),
                           categoryForSeverity(diagnostic.severity))
    , m_diagnostic(diagnostic)
    , m_removedFromEditorHandler(removedHandler)
    , m_diagMgr(diagMgr)
{
    setSettingsPage(CppEditor::Constants::CPP_CODE_MODEL_SETTINGS_ID);

    const bool warning = isWarningOrNote(diagnostic.severity);
    setDefaultToolTip(warning ? QApplication::translate("Clang Code Model Marks", "Code Model Warning")
                              : QApplication::translate("Clang Code Model Marks", "Code Model Error"));
    setPriority(warning ? TextEditor::TextMark::NormalPriority
                        : TextEditor::TextMark::HighPriority);
    updateIcon();
    if (fullVisualization) {
        setLineAnnotation(diagnosticCategoryPrefixRemoved(diagnostic.text.toString()));
        setColor(warning ? Theme::CodeModel_Warning_TextMarkColor
                         : Theme::CodeModel_Error_TextMarkColor);
    }

    // Copy to clipboard action
    QVector<QAction *> actions;
    QAction *action = new QAction();
    action->setIcon(QIcon::fromTheme("edit-copy", Icons::COPY.icon()));
    action->setToolTip(QApplication::translate("Clang Code Model Marks", "Copy to Clipboard"));
    QObject::connect(action, &QAction::triggered, [diagnostic]() {
        const QString text = ClangDiagnosticWidget::createText({diagnostic},
                                                               ClangDiagnosticWidget::InfoBar);
        QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    });
    actions << action;

    // Remove diagnostic warning action
    ProjectExplorer::Project *project = projectForCurrentEditor();
    if (project && isDiagnosticConfigChangable(project, diagnostic)) {
        action = new QAction();
        action->setIcon(Icons::BROKEN.icon());
        action->setToolTip(QApplication::translate("Clang Code Model Marks",
                                                   "Disable Diagnostic in Current Project"));
        QObject::connect(action, &QAction::triggered, [diagnostic]() {
            disableDiagnosticInCurrentProjectConfig(diagnostic);
        });
        actions << action;
    }

    setActions(actions);
}

void ClangTextMark::updateIcon(bool valid)
{
    using namespace Icons;
    if (isWarningOrNote(m_diagnostic.severity))
        setIcon(valid ? CODEMODEL_WARNING.icon() : CODEMODEL_DISABLED_WARNING.icon());
    else
        setIcon(valid ? CODEMODEL_ERROR.icon() : CODEMODEL_DISABLED_ERROR.icon());
}

bool ClangTextMark::addToolTipContent(QLayout *target) const
{
    const auto canApplyFixIt = [diag = m_diagnostic, diagMgr = m_diagMgr, c = color()] {
        return c != Utils::Theme::Color::IconsDisabledColor
                && !diagMgr->diagnosticsInvalidated()
                && diagMgr->diagnosticsWithFixIts().contains(diag);
    };
    QWidget *widget = ClangDiagnosticWidget::createWidget(
                {m_diagnostic}, ClangDiagnosticWidget::ToolTip, canApplyFixIt, "libclang");
    target->addWidget(widget);

    return true;
}

void ClangTextMark::removedFromEditor()
{
    QTC_ASSERT(m_removedFromEditorHandler, return);
    m_removedFromEditorHandler(this);
}

ClangBackEnd::DiagnosticSeverity convertSeverity(DiagnosticSeverity src)
{
    if (src == DiagnosticSeverity::Error)
        return ClangBackEnd::DiagnosticSeverity::Error;
    if (src == DiagnosticSeverity::Warning)
        return ClangBackEnd::DiagnosticSeverity::Warning;
    return ClangBackEnd::DiagnosticSeverity::Note;
}

ClangBackEnd::SourceRangeContainer convertRange(const FilePath &filePath, const Range &src)
{
    const ClangBackEnd::SourceLocationContainer start(filePath.toString(), src.start().line() + 1,
                                                      src.start().character() + 1);
    const ClangBackEnd::SourceLocationContainer end(filePath.toString(), src.end().line() + 1,
                                                      src.end().character() + 1);
    return ClangBackEnd::SourceRangeContainer(start, end);
}

ClangBackEnd::DiagnosticContainer convertDiagnostic(const ClangdDiagnostic &src,
                                                    const FilePath &filePath)
{
    ClangBackEnd::DiagnosticContainer target;
    target.ranges.append(convertRange(filePath, src.range()));
    target.location = target.ranges.first().start;
    target.text = src.message();
    target.category = src.category();
    if (src.severity())
        target.severity = convertSeverity(*src.severity());
    const Diagnostic::Code code = src.code().value_or(Diagnostic::Code());
    const QString * const codeString = Utils::get_if<QString>(&code);
    if (codeString && codeString->startsWith("-W"))
        target.enableOption = *codeString;
    for (const CodeAction &codeAction : src.codeActions().value_or(QList<CodeAction>())) {
        const Utils::optional<WorkspaceEdit> edit = codeAction.edit();
        if (!edit)
            continue;
        const Utils::optional<WorkspaceEdit::Changes> changes = edit->changes();
        if (!changes)
            continue;
        for (auto it = changes->cbegin(); it != changes->cend(); ++it) {
            for (const TextEdit &textEdit : it.value()) {
                target.fixIts << ClangBackEnd::FixItContainer(textEdit.newText(),
                        convertRange(it.key().toFilePath(), textEdit.range()));
            }
        }
    }
    return target;
}

ClangdTextMark::ClangdTextMark(const FilePath &filePath,
                               const Diagnostic &diagnostic,
                               const Client *client)
    : TextEditor::TextMark(filePath, int(diagnostic.range().start().line() + 1), client->id())
    , m_lspDiagnostic(diagnostic)
    , m_diagnostic(convertDiagnostic(ClangdDiagnostic(diagnostic), filePath))
    , m_client(client)
{
    setSettingsPage(CppEditor::Constants::CPP_CODE_MODEL_SETTINGS_ID);

    const bool isError = diagnostic.severity()
            && *diagnostic.severity() == DiagnosticSeverity::Error;
    setDefaultToolTip(isError ? tr("Code Model Error") : tr("Code Model Warning"));
    setPriority(isError ? TextEditor::TextMark::HighPriority
                        : TextEditor::TextMark::NormalPriority);
    setIcon(isError ? Icons::CODEMODEL_ERROR.icon() : Icons::CODEMODEL_WARNING.icon());
    if (client->project()) {
        setLineAnnotation(diagnostic.message());
        setColor(isError ? Theme::CodeModel_Error_TextMarkColor
                         : Theme::CodeModel_Warning_TextMarkColor);
        ClangDiagnosticManager::addTask(m_diagnostic);
    }

    // Copy to clipboard action
    QVector<QAction *> actions;
    QAction *action = new QAction();
    action->setIcon(QIcon::fromTheme("edit-copy", Icons::COPY.icon()));
    action->setToolTip(tr("Clang Code Model Marks", "Copy to Clipboard"));
    QObject::connect(action, &QAction::triggered, [diag = m_diagnostic]() {
        const QString text = ClangDiagnosticWidget::createText({diag},
                                                               ClangDiagnosticWidget::InfoBar);
        QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    });
    actions << action;

    // Remove diagnostic warning action
    ProjectExplorer::Project *project = projectForCurrentEditor();
    if (project && isDiagnosticConfigChangable(project, m_diagnostic)) {
        action = new QAction();
        action->setIcon(Icons::BROKEN.icon());
        action->setToolTip(tr("Disable Diagnostic in Current Project"));
        QObject::connect(action, &QAction::triggered, [diag = m_diagnostic]() {
            disableDiagnosticInCurrentProjectConfig(diag);
        });
        actions << action;
    }

    setActions(actions);
}

bool ClangdTextMark::addToolTipContent(QLayout *target) const
{
    const auto canApplyFixIt = [c = m_client, diag = m_lspDiagnostic, fp = fileName()] {
        return QTC_GUARD(c) && c->reachable()
               && c->hasDiagnostic(DocumentUri::fromFilePath(fp), diag);
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
