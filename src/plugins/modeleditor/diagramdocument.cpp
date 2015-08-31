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

#include "diagramdocument.h"

#include "modeleditor_constants.h"
#include "modeleditor_plugin.h"
#include "modelsmanager.h"
#include "modeldocument.h"
#include "extdocumentcontroller.h"

#include "qmt/serializer/diagramreferenceserializer.h"
#include "qmt/project_controller/projectcontroller.h"
#include "qmt/project/project.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/model/mdiagram.h"
#include "qmt/controller/namecontroller.h"

#include <coreplugin/documentmanager.h>

#include <utils/fileutils.h>

#include <QFileInfo>

namespace ModelEditor {
namespace Internal {

class DiagramDocument::DiagramDocumentPrivate {
public:
    ExtDocumentController *documentController = 0;
    qmt::Uid diagramUid;
};

DiagramDocument::DiagramDocument(QObject *parent)
    : Core::IDocument(parent),
      d(new DiagramDocumentPrivate)
{
    setId(Constants::DIAGRAM_EDITOR_ID);
    setMimeType(QLatin1String(Constants::MIME_TYPE_DIAGRAM_REFERENCE));
}

DiagramDocument::~DiagramDocument()
{
    if (d->documentController)
        ModelEditorPlugin::modelsManager()->release(d->documentController, this);
    delete d;
}

bool DiagramDocument::save(QString *errorString, const QString &name, bool autoSave)
{
    Q_UNUSED(autoSave);

    if (!d->documentController) {
        *errorString = tr("No model loaded. Cannot save diagram's model file.");
        return false;
    }

    // "Save As" is not allowed"
    if (!name.isEmpty() && name != filePath().toString()) {
        *errorString = tr("Cannot save diagrams to file.");
        return false;
    }

    ModelDocument *modelDocument = ModelEditorPlugin::modelsManager()->findModelDocument(d->documentController);
    if (modelDocument) {
        Core::DocumentManager::saveDocument(modelDocument);
    } else {
        try {
            if (d->documentController->getProjectController()->isModified())
                d->documentController->getProjectController()->save();
        } catch (const qmt::Exception &ex) {
            *errorString = ex.getErrorMsg();
            return false;
        }
    }

    onDiagramRenamed();

    return true;
}

bool DiagramDocument::setContents(const QByteArray &contents)
{
    // load reference file
    qmt::DiagramReferenceSerializer serializer;
    qmt::DiagramReferenceSerializer::Reference reference = serializer.load(contents);

    // this should never fail because the contents was generated from actual model and diagram
    d->documentController = ModelEditorPlugin::modelsManager()->findOrLoadModel(reference._model_uid, this);
    if (!d->documentController)
        return false;
    d->diagramUid = reference._diagram_uid;
    qmt::MDiagram *diagram = d->documentController->getModelController()->findObject<qmt::MDiagram>(d->diagramUid);
    if (!diagram)
        return false;

    connect(d->documentController, &qmt::DocumentController::changed, this, &IDocument::changed);

    setTemporary(true);

    // set document's file path to avoid asking for file path on saving document
    // TODO encode model and diagram UIDs in path
    // (allowing open from file name in DiagramEditor which is called
    // if diagram shall be opened from navigation history)
    QFileInfo fileInfo(d->documentController->getProjectController()->getProject()->getFileName());
    QString filePath = fileInfo.path() + QStringLiteral("/")
            + qmt::NameController::convertElementNameToBaseFileName(diagram->getName())
            + QStringLiteral(".qdiagram");
    setFilePath(Utils::FileName::fromString(filePath));

    onDiagramRenamed();

    emit contentSet();

    return true;
}

bool DiagramDocument::isFileReadOnly() const
{
    ModelDocument *modelDocument = ModelEditorPlugin::modelsManager()->findModelDocument(d->documentController);
    if (modelDocument)
        return modelDocument->isFileReadOnly();
    if (isTemporary())
        return false;
    return Core::IDocument::isFileReadOnly();
}

QString DiagramDocument::defaultPath() const
{
    return QLatin1String(".");
}

QString DiagramDocument::suggestedFileName() const
{
    return tr("diagram.qdiagram");
}

bool DiagramDocument::isModified() const
{
    return d->documentController ? d->documentController->getProjectController()->isModified() : false;
}

bool DiagramDocument::isSaveAsAllowed() const
{
    return false;
}

bool DiagramDocument::reload(QString *errorString, Core::IDocument::ReloadFlag flag,
                             Core::IDocument::ChangeType type)
{
    if (flag == FlagIgnore)
        return true;
    if (type == TypePermissions) {
        emit changed();
        return true;
    }
    *errorString = tr("Cannot reload diagram file.");
    return false;
}

ExtDocumentController *DiagramDocument::documentController() const
{
    return d->documentController;
}

qmt::Uid DiagramDocument::diagramUid() const
{
    return d->diagramUid;
}

void DiagramDocument::onDiagramRenamed()
{
    qmt::MDiagram *diagram = d->documentController->getModelController()->findObject<qmt::MDiagram>(d->diagramUid);
    if (diagram) {
        QFileInfo fileInfo(d->documentController->getProjectController()->getProject()->getFileName());
        setPreferredDisplayName(tr("%1 [%2]").arg(diagram->getName()).arg(fileInfo.fileName()));
    } else {
        setPreferredDisplayName(QString());
    }
}

} // namespace Internal
} // namespace ModelEditor
