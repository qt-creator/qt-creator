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
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model/mdiagram.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"

#include <utils/id.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QDir>

namespace ModelEditor {
namespace Internal {

class ModelDocument::ModelDocumentPrivate {
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

Core::IDocument::OpenResult ModelDocument::open(QString *errorString,
                                                const Utils::FilePath &filePath,
                                                const Utils::FilePath &realFilePath)
{
    Q_UNUSED(filePath)

    OpenResult result = load(errorString, realFilePath.toString());
    return result;
}

bool ModelDocument::saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave)
{
    if (!d->documentController) {
        *errorString = Tr::tr("No model loaded. Cannot save.");
        return false;
    }

    d->documentController->projectController()->setFileName(filePath.toString());
    try {
        d->documentController->projectController()->save();
    } catch (const qmt::Exception &ex) {
        *errorString = ex.errorMessage();
        return false;
    }

    if (autoSave) {
        d->documentController->projectController()->setModified();
    } else {
        setFilePath(Utils::FilePath::fromString(d->documentController->projectController()->project()->fileName()));
        emit changed();
    }

    return true;
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

bool ModelDocument::reload(QString *errorString, Core::IDocument::ReloadFlag flag,
                           Core::IDocument::ChangeType type)
{
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return true;
    try {
        d->documentController->loadProject(filePath().toString());
    } catch (const qmt::FileNotFoundException &ex) {
        *errorString = ex.errorMessage();
        return false;
    } catch (const qmt::Exception &ex) {
        *errorString = Tr::tr("Could not open \"%1\" for reading: %2.").arg(filePath().toString()).arg(ex.errorMessage());
        return false;
    }
    emit contentSet();
    return true;
}

ExtDocumentController *ModelDocument::documentController() const
{
    return d->documentController;
}

Core::IDocument::OpenResult ModelDocument::load(QString *errorString, const QString &fileName)
{
    d->documentController = ModelEditorPlugin::modelsManager()->createModel(this);
    connect(d->documentController, &qmt::DocumentController::changed, this, &IDocument::changed);

    try {
        d->documentController->loadProject(fileName);
        setFilePath(Utils::FilePath::fromString(d->documentController->projectController()->project()->fileName()));
    } catch (const qmt::FileNotFoundException &ex) {
        *errorString = ex.errorMessage();
        return OpenResult::ReadError;
    } catch (const qmt::Exception &ex) {
        *errorString = Tr::tr("Could not open \"%1\" for reading: %2.").arg(fileName).arg(ex.errorMessage());
        return OpenResult::CannotHandle;
    }

    QString configPath = d->documentController->projectController()->project()->configPath();
    if (!configPath.isEmpty()) {
        QString canonicalPath = QFileInfo(QDir(QFileInfo(fileName).path()).filePath(configPath)).canonicalFilePath();
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

} // namespace Internal
} // namespace ModelEditor
