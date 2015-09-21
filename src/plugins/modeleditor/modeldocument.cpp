/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "modeldocument.h"

#include "modeleditor_constants.h"
#include "modeleditor_plugin.h"
#include "modelsmanager.h"
#include "extdocumentcontroller.h"

#include "qmt/infrastructure/ioexceptions.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model/mdiagram.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"

#include <coreplugin/id.h>
#include <utils/fileutils.h>

#include <QFileInfo>

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

    if (!load(errorString, realFileName))
        return Core::IDocument::OpenResult::ReadError;
    return Core::IDocument::OpenResult::Success;
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
    d->documentController->getProjectController()->setFileName(actualName);
    try {
        d->documentController->getProjectController()->save();
    } catch (const qmt::Exception &ex) {
        *errorString = ex.getErrorMsg();
        return false;
    }

    if (!autoSave) {
        setFilePath(Utils::FileName::fromString(d->documentController->getProjectController()->getProject()->getFileName()));
        emit changed();
    }

    return true;
}

QString ModelDocument::defaultPath() const
{
    return QLatin1String(".");
}

QString ModelDocument::suggestedFileName() const
{
    return tr("model.qmodel");
}

bool ModelDocument::isModified() const
{
    return d->documentController ? d->documentController->getProjectController()->isModified() : false;
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

bool ModelDocument::load(QString *errorString, const QString &fileName)
{
    d->documentController = ModelEditorPlugin::modelsManager()->createModel(this);
    connect(d->documentController, &qmt::DocumentController::changed, this, &IDocument::changed);

    try {
        d->documentController->loadProject(fileName);
        setFilePath(Utils::FileName::fromString(d->documentController->getProjectController()->getProject()->getFileName()));
    } catch (const qmt::Exception &ex) {
        *errorString = ex.getErrorMsg();
        return false;
    }

    emit contentSet();
    return true;
}

} // namespace Internal
} // namespace ModelEditor
