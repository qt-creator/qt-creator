// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstracteditorsupport.h"

#include "cppeditorplugin.h"
#include "cppeditortr.h"
#include "cppfilesettingspage.h"
#include "cppmodelmanager.h"

#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/templateengine.h>

using namespace Utils;

namespace CppEditor {

AbstractEditorSupport::AbstractEditorSupport(QObject *parent) :
    QObject(parent), m_revision(1)
{
    CppModelManager::addExtraEditorSupport(this);
}

AbstractEditorSupport::~AbstractEditorSupport()
{
    CppModelManager::removeExtraEditorSupport(this);
}

void AbstractEditorSupport::updateDocument()
{
    ++m_revision;
    CppModelManager::updateSourceFiles({filePath()});
}

void AbstractEditorSupport::notifyAboutUpdatedContents() const
{
    CppModelManager::emitAbstractEditorSupportContentsUpdated(
                filePath().toString(), sourceFilePath().toString(), contents());
}

QString AbstractEditorSupport::licenseTemplate(ProjectExplorer::Project *project,
                                               const FilePath &filePath, const QString &className)
{
    const QString license = Internal::CppEditorPlugin::licenseTemplate(project);
    Utils::MacroExpander expander;
    expander.registerVariable("Cpp:License:FileName", Tr::tr("The file name."),
                              [filePath] { return filePath.fileName(); });
    expander.registerVariable("Cpp:License:ClassName", Tr::tr("The class name."),
                              [className] { return className; });

    return Utils::TemplateEngine::processText(&expander, license, nullptr);
}

bool AbstractEditorSupport::usePragmaOnce(ProjectExplorer::Project *project)
{
    return Internal::CppEditorPlugin::usePragmaOnce(project);
}

} // CppEditor
