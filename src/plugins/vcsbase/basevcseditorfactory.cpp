// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basevcseditorfactory.h"

#include "vcsbaseeditor.h"
#include "vcsbasetr.h"

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textdocument.h>

#include <diffeditor/diffeditorconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QStringList>

using namespace TextEditor;

/*!
    \class VcsBase::BaseVCSEditorFactory

    \brief The BaseVCSEditorFactory class is the base class for editor
    factories creating instances of VcsBaseEditor subclasses.

    \sa VcsBase::VcsBaseEditorWidget
*/

namespace VcsBase {

VcsEditorFactory::VcsEditorFactory(const VcsBaseEditorParameters *parameters,
                                   // Force copy, see QTCREATORBUG-13218
                                   const EditorWidgetCreator editorWidgetCreator,
                                   std::function<void (const Utils::FilePath &, const QString &)> describeFunc)
{
    setId(parameters->id);
    setDisplayName(Tr::tr(parameters->displayName));
    if (QLatin1String(parameters->mimeType) != QLatin1String(DiffEditor::Constants::DIFF_EDITOR_MIMETYPE))
        addMimeType(QLatin1String(parameters->mimeType));

    setEditorActionHandlers(TextEditorActionHandler::None);
    setDuplicatedSupported(false);

    setDocumentCreator([parameters] {
        auto document = new TextDocument(parameters->id);
        document->setMimeType(QLatin1String(parameters->mimeType));
        document->setSuspendAllowed(false);
        return document;
    });

    setEditorWidgetCreator([parameters, editorWidgetCreator, describeFunc] {
        auto widget = editorWidgetCreator();
        auto editorWidget = Aggregation::query<VcsBaseEditorWidget>(widget);
        editorWidget->setDescribeFunc(describeFunc);
        editorWidget->setParameters(parameters);
        return widget;
    });

    setEditorCreator([] { return new VcsBaseEditor(); });
    setMarksVisible(false);
}

VcsEditorFactory::~VcsEditorFactory() = default;

} // namespace VcsBase
