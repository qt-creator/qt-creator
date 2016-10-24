/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "modeldocument.h"

#include "modeleditor_constants.h"
#include "modeleditor_plugin.h"
#include "modelsmanager.h"
#include "extdocumentcontroller.h"

#include "qmt/config/configcontroller.h"
#include "qmt/infrastructure/ioexceptions.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model/mdiagram.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"

#include <coreplugin/id.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QDir>

namespace ModelEditor {
namespace Internal {

class ModelDocument::ModelDocumentPrivate {
public:
    ExtDocumentController *documentController = 0;
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

Core::IDocument::OpenResult ModelDocument::open(QString *errorString, const QString &fileName,
                                                const QString &realFileName)
{
    Q_UNUSED(fileName);

    OpenResult result = load(errorString, realFileName);
    return result;
}

bool ModelDocument::save(QString *errorString, const QString &name, bool autoSave)
{
    if (!d->documentController) {
        *errorString = tr("No model loaded. Cannot save.");
        return false;
    }

    QString actualName = filePath().toString();
    if (!name.isEmpty())
        actualName = name;
    d->documentController->projectController()->setFileName(actualName);
    try {
        d->documentController->projectController()->save();
    } catch (const qmt::Exception &ex) {
        *errorString = ex.errorMessage();
        return false;
    }

    if (autoSave) {
        d->documentController->projectController()->setModified();
    } else {
        setFilePath(Utils::FileName::fromString(d->documentController->projectController()->project()->fileName()));
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
    if (flag == FlagIgnore)
        return true;
    if (type == TypePermissions) {
        emit changed();
        return true;
    }
    *errorString = tr("Cannot reload model file.");
    return false;
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
        setFilePath(Utils::FileName::fromString(d->documentController->projectController()->project()->fileName()));
    } catch (const qmt::FileNotFoundException &ex) {
        *errorString = ex.errorMessage();
        return OpenResult::ReadError;
    } catch (const qmt::Exception &ex) {
        *errorString = tr("Could not open \"%1\" for reading: %2.").arg(fileName).arg(ex.errorMessage());
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
