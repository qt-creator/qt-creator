// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeldocument.h"

#include "extdocumentcontroller.h"
#include "modeleditor_constants.h"
#include "modeleditor_plugin.h"
#include "modeleditortr.h"
#include "modelsmanager.h"

#include "qmt/config/configcontroller.h"
#include "qmt/infrastructure/ioexceptions.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"

#include <utils/id.h>
#include <utils/fileutils.h>

using namespace Utils;

namespace ModelEditor::Internal {

class ModelDocument::ModelDocumentPrivate
{
public:
    ExtDocumentController *documentController = nullptr;
};

ModelDocument::ModelDocument(QObject *parent)
    : Core::IDocument(parent),
      d(new ModelDocumentPrivate)
{
    setId(ModelEditor::Constants::MODEL_EDITOR_ID);
    setMimeType(QLatin1String(Constants::MIME_TYPE_MODEL));
}

ModelDocument::~ModelDocument()
{
    if (d->documentController)
        ModelEditorPlugin::modelsManager()->releaseModel(d->documentController);
    delete d;
}

Core::IDocument::OpenResult ModelDocument::open(const FilePath &filePath,
                                                const FilePath &realFilePath)
{
    Q_UNUSED(filePath)

    OpenResult result = load(realFilePath);
    return result;
}

Result<> ModelDocument::saveImpl(const FilePath &filePath, bool autoSave)
{
    if (!d->documentController)
        return ResultError(Tr::tr("No model loaded. Cannot save."));

    d->documentController->projectController()->setFileName(filePath);
    try {
        d->documentController->projectController()->save();
    } catch (const qmt::Exception &ex) {
        return ResultError(ex.errorMessage());
    }

    if (autoSave) {
        d->documentController->projectController()->setModified();
    } else {
        setFilePath(d->documentController->projectController()->project()->fileName());
        emit changed();
    }

    return ResultOk;
}

bool ModelDocument::shouldAutoSave() const
{
    return isModified();
}

bool ModelDocument::isModified() const
{
    return d->documentController ? d->documentController->projectController()->isModified() : false;
}

bool ModelDocument::isSaveAsAllowed() const
{
    return true;
}

Result<> ModelDocument::reload(Core::IDocument::ReloadFlag flag,
                             Core::IDocument::ChangeType type)
{
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return ResultOk;
    try {
        d->documentController->loadProject(filePath());
    } catch (const qmt::FileNotFoundException &ex) {
        return ResultError(ex.errorMessage());
    } catch (const qmt::Exception &ex) {
        return ResultError(Tr::tr("Could not open \"%1\" for reading: %2.")
                           .arg(filePath().toUserOutput(), ex.errorMessage()));
    }
    emit contentSet();
    return ResultOk;
}

ExtDocumentController *ModelDocument::documentController() const
{
    return d->documentController;
}

Core::IDocument::OpenResult ModelDocument::load(const FilePath &fileName)
{
    d->documentController = ModelEditorPlugin::modelsManager()->createModel(this);
    connect(d->documentController, &qmt::DocumentController::changed, this, &IDocument::changed);

    try {
        d->documentController->loadProject(fileName);
        setFilePath(d->documentController->projectController()->project()->fileName());
    } catch (const qmt::FileNotFoundException &ex) {
        return {OpenResult::CannotHandle, ex.errorMessage()};
    } catch (const qmt::Exception &ex) {
        return {OpenResult::CannotHandle,
                    Tr::tr("Could not open \"%1\" for reading: %2.")
                    .arg(fileName.toUserOutput(), ex.errorMessage())};
    }

    FilePath configPath = d->documentController->projectController()->project()->configPath();
    if (!configPath.isEmpty()) {
        FilePath canonicalPath =fileName.absolutePath().resolvePath(configPath);
        if (!canonicalPath.isEmpty()) {
            // TODO error output on reading definition files
            d->documentController->configController()->readStereotypeDefinitions(canonicalPath);
        } else {
            // TODO error output
        }
    }

    emit contentSet();
    return OpenResult::Success;
}

} // namespace ModelEditor::Internal
