// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectsmapeditor.h"

#include "objectsmapdocument.h"
#include "objectsmapeditorwidget.h"
#include "squishconstants.h"
#include "squishtr.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>

using namespace Core;

namespace Squish::Internal {

class ObjectsMapEditor final : public IEditor
{
public:
    ObjectsMapEditor(std::shared_ptr<ObjectsMapDocument> document)
        : m_document(document)
    {
        setWidget(new ObjectsMapEditorWidget(m_document.get()));
        setDuplicateSupported(true);
    }
    ~ObjectsMapEditor() final { delete m_widget; }

private:
    IDocument *document() const override { return m_document.get(); }
    QWidget *toolBar() final { return nullptr; }
    IEditor *duplicate() final { return new ObjectsMapEditor(m_document); }
    std::shared_ptr<ObjectsMapDocument> m_document;
};

class ObjectsMapEditorFactory final : public IEditorFactory
{
public:
    ObjectsMapEditorFactory()
    {
        setId(Constants::OBJECTSMAP_EDITOR_ID);
        setDisplayName(Tr::tr("Squish Object Map Editor"));
        addMimeType(Constants::SQUISH_OBJECTSMAP_MIMETYPE);
        setEditorCreator([] {
            return new ObjectsMapEditor(std::shared_ptr<ObjectsMapDocument>(new ObjectsMapDocument));
        });
    }
};

void setupObjectsMapEditor()
{
    static ObjectsMapEditorFactory theObjectsMapEditorFactory;
}

} // Squish::Internal
