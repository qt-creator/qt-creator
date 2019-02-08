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
#include "clangdiagnostictooltipwidget.h"
#include "clangeditordocumentprocessor.h"
#include "clangmodelmanagersupport.h"
#include "clangprojectsettings.h"
#include "clangutils.h"

#include <coreplugin/icore.h>
#include <cpptools/clangdiagnosticconfigsmodel.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppcodemodelsettings.h>

#include <utils/fadingindicator.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLayout>
#include <QString>

using namespace Utils;

namespace ClangCodeModel {

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

static Core::Id categoryForSeverity(ClangBackEnd::DiagnosticSeverity severity)
{
    return isWarningOrNote(severity) ? Constants::CLANG_WARNING : Constants::CLANG_ERROR;
}

ProjectExplorer::Project *projectForCurrentEditor()
{
    using namespace CppTools;
    using namespace ClangCodeModel::Internal;

    const QString filePath = Utils::currentCppEditorDocumentFilePath();
    if (filePath.isEmpty())
        return nullptr;

    if (auto processor = ClangEditorDocumentProcessor::get(filePath)) {
        if (ProjectPart::Ptr projectPart = processor->projectPart())
            return projectPart->project;
    }

    return nullptr;
}

void disableDiagnosticInConfig(CppTools::ClangDiagnosticConfig &config,
                               const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    // Clang check
    if (!diagnostic.disableOption.isEmpty()) {
        config.setClangOptions(config.clangOptions() + QStringList(diagnostic.disableOption));
        return;
    }

    // Clazy check
    using namespace ClangCodeModel::Utils;
    DiagnosticTextInfo textInfo(diagnostic.text);
    if (DiagnosticTextInfo::isClazyOption(textInfo.option())) {
        const QString checkName = DiagnosticTextInfo::clazyCheckName(textInfo.option());
        QStringList newChecks = config.clazyChecks().split(',');
        newChecks.removeOne(checkName);
        config.setClazyChecks(newChecks.join(','));
        return;
    }

    // Tidy check
    config.setClangTidyChecks(config.clangTidyChecks() + QString(",-") + textInfo.option());
}

void disableDiagnosticInCurrentProjectConfig(const ClangBackEnd::DiagnosticContainer &diagnostic)
{
    using namespace CppTools;
    using namespace ClangCodeModel::Internal;

    ProjectExplorer::Project *project = projectForCurrentEditor();
    QTC_ASSERT(project, return );

    // Get settings
    ClangProjectSettings &projectSettings = ClangModelManagerSupport::instance()->projectSettings(
        project);
    const QSharedPointer<CppCodeModelSettings> globalSettings = codeModelSettings();

    // Get config id
    Core::Id currentConfigId = projectSettings.warningConfigId();
    if (projectSettings.useGlobalConfig())
        currentConfigId = globalSettings->clangDiagnosticConfigId();

    // Get config
    ClangDiagnosticConfigsModel configsModel(globalSettings->clangCustomDiagnosticConfigs());
    QTC_ASSERT(configsModel.hasConfigWithId(currentConfigId), return );
    ClangDiagnosticConfig config = configsModel.configWithId(currentConfigId);

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
    ::Utils::FadingIndicator::showText(Core::ICore::mainWindow(),
                                       text,
                                       ::Utils::FadingIndicator::SmallText);
}

} // anonymous namespace

ClangTextMark::ClangTextMark(const FileName &fileName,
                             const ClangBackEnd::DiagnosticContainer &diagnostic,
                             const RemovedFromEditorHandler &removedHandler,
                             bool fullVisualization)
    : TextEditor::TextMark(fileName,
                           int(diagnostic.location.line),
                           categoryForSeverity(diagnostic.severity))
    , m_diagnostic(diagnostic)
    , m_removedFromEditorHandler(removedHandler)
{
    const bool warning = isWarningOrNote(diagnostic.severity);
    setDefaultToolTip(warning ? QApplication::translate("Clang Code Model Marks", "Code Model Warning")
                              : QApplication::translate("Clang Code Model Marks", "Code Model Error"));
    setPriority(warning ? TextEditor::TextMark::NormalPriority
                        : TextEditor::TextMark::HighPriority);
    updateIcon();
    if (fullVisualization) {
        setLineAnnotation(Utils::diagnosticCategoryPrefixRemoved(diagnostic.text.toString()));
        setColor(warning ? ::Utils::Theme::CodeModel_Warning_TextMarkColor
                         : ::Utils::Theme::CodeModel_Error_TextMarkColor);
    }

    // Copy to clipboard action
    QVector<QAction *> actions;
    QAction *action = new QAction();
    action->setIcon(QIcon::fromTheme("edit-copy", ::Utils::Icons::COPY.icon()));
    QObject::connect(action, &QAction::triggered, [diagnostic]() {
        using namespace ClangCodeModel::Internal;
        const QString text = ClangDiagnosticWidget::createText({diagnostic},
                                                               ClangDiagnosticWidget::InfoBar);
        QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    });
    actions << action;

    // Remove diagnostic warning action
    if (projectForCurrentEditor()) {
        action = new QAction();
        action->setIcon(::Utils::Icons::BROKEN.icon());
        QObject::connect(action, &QAction::triggered, [diagnostic]() {
            disableDiagnosticInCurrentProjectConfig(diagnostic);
        });
        actions << action;
    }

    setActions(actions);
}

void ClangTextMark::updateIcon(bool valid)
{
    using namespace ::Utils::Icons;
    if (isWarningOrNote(m_diagnostic.severity))
        setIcon(valid ? CODEMODEL_WARNING.icon() : CODEMODEL_DISABLED_WARNING.icon());
    else
        setIcon(valid ? CODEMODEL_ERROR.icon() : CODEMODEL_DISABLED_ERROR.icon());
}

bool ClangTextMark::addToolTipContent(QLayout *target) const
{
    using Internal::ClangDiagnosticWidget;

    QWidget *widget = ClangDiagnosticWidget::createWidget({m_diagnostic},
                                                          ClangDiagnosticWidget::ToolTip);
    target->addWidget(widget);

    return true;
}

void ClangTextMark::removedFromEditor()
{
    QTC_ASSERT(m_removedFromEditorHandler, return);
    m_removedFromEditorHandler(this);
}

} // namespace ClangCodeModel

