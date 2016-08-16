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

#include "clangcodemodelplugin.h"

#include "clangconstants.h"
#include "clangprojectsettingswidget.h"

#ifdef WITH_TESTS
#  include "test/clangcodecompletion_test.h"
#endif

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <texteditor/textmark.h>

namespace ClangCodeModel {
namespace Internal {

namespace {

void initializeTextMarks()
{
    TextEditor::TextMark::setCategoryColor(Core::Id(Constants::CLANG_WARNING),
                                           Utils::Theme::ClangCodeModel_Warning_TextMarkColor);
    TextEditor::TextMark::setCategoryColor(Core::Id(Constants::CLANG_ERROR),
                                           Utils::Theme::ClangCodeModel_Error_TextMarkColor);
    TextEditor::TextMark::setDefaultToolTip(Core::Id(Constants::CLANG_WARNING),
                                            QApplication::translate("Clang Code Model Marks",
                                                                    "Code Model Warning"));
    TextEditor::TextMark::setDefaultToolTip(Core::Id(Constants::CLANG_ERROR),
                                            QApplication::translate("Clang Code Model Marks",
                                                                    "Code Model Error"));
}

void addProjectPanelWidget()
{
    auto panelFactory = new ProjectExplorer::ProjectPanelFactory();
    panelFactory->setPriority(60);
    panelFactory->setDisplayName(ClangProjectSettingsWidget::tr("Clang Code Model"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new ClangProjectSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
}

} // anonymous namespace

bool ClangCodeModelPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    CppTools::CppModelManager::instance()->activateClangCodeModel(&m_modelManagerSupportProvider);

    initializeTextMarks();
    addProjectPanelWidget();

    return true;
}

void ClangCodeModelPlugin::extensionsInitialized()
{
}

#ifdef WITH_TESTS
QList<QObject *> ClangCodeModelPlugin::createTestObjects() const
{
    return {
        new Tests::ClangCodeCompletionTest,
    };
}
#endif


} // namespace Internal
} // namespace Clang
