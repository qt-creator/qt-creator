// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstracteditorsupport.h"

#include "cppeditorplugin.h"
#include "cppfilesettingspage.h"
#include "cppmodelmanager.h"

#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/templateengine.h>

using namespace Utils;

namespace CppEditor {

AbstractEditorSupport::AbstractEditorSupport(CppModelManager *modelmanager, QObject *parent) :
    QObject(parent), m_modelmanager(modelmanager), m_revision(1)
{
    modelmanager->addExtraEditorSupport(this);
}

AbstractEditorSupport::~AbstractEditorSupport()
{
    m_modelmanager->removeExtraEditorSupport(this);
}

void AbstractEditorSupport::updateDocument()
{
    ++m_revision;
    m_modelmanager->updateSourceFiles(QSet<QString>{filePath().toString()});
}

void AbstractEditorSupport::notifyAboutUpdatedContents() const
{
    m_modelmanager->emitAbstractEditorSupportContentsUpdated(
                filePath().toString(), sourceFilePath().toString(), contents());
}

QString AbstractEditorSupport::licenseTemplate(const FilePath &filePath, const QString &className)
{
    const QString license = Internal::CppFileSettings::licenseTemplate();
    Utils::MacroExpander expander;
    expander.registerVariable("Cpp:License:FileName", tr("The file name."),
                              [filePath] { return filePath.fileName(); });
    expander.registerVariable("Cpp:License:ClassName", tr("The class name."),
                              [className] { return className; });

    return Utils::TemplateEngine::processText(&expander, license, nullptr);
}

bool AbstractEditorSupport::usePragmaOnce()
{
    return Internal::CppEditorPlugin::usePragmaOnce();
}

} // CppEditor
