// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectsmapeditor.h"

#include "objectsmapdocument.h"
#include "objectsmapeditorwidget.h"
#include "squishconstants.h"
#include "squishtr.h"

#include <coreplugin/editormanager/ieditor.h>

namespace Squish::Internal {

class ObjectsMapEditor : public Core::IEditor
{
public:
    ObjectsMapEditor(std::shared_ptr<ObjectsMapDocument> document)
        : m_document(document)
    {
        setWidget(new ObjectsMapEditorWidget(m_document.get()));
        setDuplicateSupported(true);
    }
    ~ObjectsMapEditor() override { delete m_widget; }

private:
    Core::IDocument *document() const override { return m_document.get(); }
    QWidget *toolBar() override { return nullptr; }
    Core::IEditor *duplicate() override { return new ObjectsMapEditor(m_document); }
    std::shared_ptr<ObjectsMapDocument> m_document;
};


// Factory

ObjectsMapEditorFactory::ObjectsMapEditorFactory()
{
    setId(Constants::OBJECTSMAP_EDITOR_ID);
    setDisplayName(Tr::tr("Squish Object Map Editor"));
    addMimeType(Constants::SQUISH_OBJECTSMAP_MIMETYPE);
    setEditorCreator([] {
        return new ObjectsMapEditor(std::shared_ptr<ObjectsMapDocument>(new ObjectsMapDocument));
    });
}

} // Squish::Internal
