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

#include "clangformatplugin.h"

#include "clangformatconfigwidget.h"
#include "clangformatconstants.h"
#include "clangformatindenter.h"
#include "clangformatutils.h"
#include "tests/clangformat-test.h"

#include <utils/qtcassert.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cppeditorconstants.h>

#include <cppeditor/cppcodestylepreferencesfactory.h>
#include <cppeditor/cppcodestylesettingspage.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/texteditorsettings.h>

#include <clang/Format/Format.h>

#include <utils/algorithm.h>
#include <utils/infobar.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangFormat {

class ClangFormatStyleFactory : public CppEditor::CppCodeStylePreferencesFactory
{
    Q_DECLARE_TR_FUNCTIONS(ClangFormatStyleFactory)
public:
    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override
    {
        return new ClangFormatIndenter(doc);
    }

    std::pair<CppEditor::CppCodeStyleWidget *, QString> additionalTab(
        ProjectExplorer::Project *project, QWidget *parent) const override
    {
        return {new ClangFormatConfigWidget(project, parent), tr("ClangFormat")};
    }
};

static void replaceCppCodeStyle()
{
    using namespace TextEditor;
    TextEditorSettings::unregisterCodeStyleFactory(CppEditor::Constants::CPP_SETTINGS_ID);
    TextEditorSettings::registerCodeStyleFactory(new ClangFormatStyleFactory);
}

bool ClangFormatPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)
    replaceCppCodeStyle();

    ActionContainer *contextMenu = ActionManager::actionContainer(CppEditor::Constants::M_CONTEXT);
    if (contextMenu) {
        auto openClangFormatConfigAction
            = new QAction(tr("Open Used .clang-format Configuration File"), this);
        Command *command = ActionManager::registerAction(openClangFormatConfigAction,
                                                         Constants::OPEN_CURRENT_CONFIG_ID);
        contextMenu->addSeparator();
        contextMenu->addAction(command);

        if (EditorManager::currentEditor()) {
            if (const IDocument *doc = EditorManager::currentEditor()->document())
                openClangFormatConfigAction->setData(doc->filePath().toVariant());
        }

        connect(openClangFormatConfigAction,
                &QAction::triggered,
                this,
                [openClangFormatConfigAction]() {
                    const FilePath fileName = FilePath::fromVariant(openClangFormatConfigAction->data());
                    if (!fileName.isEmpty())
                        EditorManager::openEditor(FilePath::fromString(configForFile(fileName)));
                });

        connect(EditorManager::instance(),
                &EditorManager::currentEditorChanged,
                this,
                [openClangFormatConfigAction](IEditor *editor) {
                    if (!editor)
                        return;

                    if (const IDocument *doc = editor->document())
                        openClangFormatConfigAction->setData(doc->filePath().toVariant());
                });
    }
#ifndef KEEP_LINE_BREAKS_FOR_NON_EMPTY_LINES_BACKPORTED
#ifdef _MSC_VER
#pragma message( \
    "ClangFormat: building against unmodified Clang, see README.md for more info")
#else
#warning ClangFormat: building against unmodified Clang, see README.md for more info
#endif
    static const Id clangFormatFormatWarningKey = "ClangFormatFormatWarning";
    if (!ICore::infoBar()->canInfoBeAdded(clangFormatFormatWarningKey))
        return true;
    InfoBarEntry
        info(clangFormatFormatWarningKey,
             tr("The ClangFormat plugin has been built against an unmodified Clang. "
                "You might experience formatting glitches in certain circumstances. "
                "See https://code.qt.io/cgit/qt-creator/qt-creator.git/tree/README.md for more "
                 "information."),
             InfoBarEntry::GlobalSuppression::Enabled);
    ICore::infoBar()->addInfo(info);
#endif
    return true;
}

QVector<QObject *> ClangFormatPlugin::createTestObjects() const
{
    return {
#ifdef WITH_TESTS
        new Internal::ClangFormatTest,
#endif
    };
}

} // namespace ClangFormat
