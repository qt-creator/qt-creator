/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "basevcseditorfactory.h"
#include "vcsbaseeditor.h"

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textdocument.h>

#include <diffeditor/diffeditorconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
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
                                   std::function<void(const QString &, const QString &)> describeFunc)
{
    setId(parameters->id);
    setDisplayName(QCoreApplication::translate("VCS", parameters->displayName));
    if (QLatin1String(parameters->mimeType) != QLatin1String(DiffEditor::Constants::DIFF_EDITOR_MIMETYPE))
        addMimeType(QLatin1String(parameters->mimeType));

    setEditorActionHandlers(TextEditorActionHandler::None);
    setDuplicatedSupported(false);

    setDocumentCreator([parameters]() -> TextDocument* {
        auto document = new TextDocument(parameters->id);
 //  if (QLatin1String(parameters->mimeType) != QLatin1String(DiffEditor::Constants::DIFF_EDITOR_MIMETYPE))
        document->setMimeType(QLatin1String(parameters->mimeType));
        document->setSuspendAllowed(false);
        return document;
    });

    setEditorWidgetCreator([parameters, editorWidgetCreator, describeFunc]() {
        auto widget = editorWidgetCreator();
        auto editorWidget = Aggregation::query<VcsBaseEditorWidget>(widget);
        editorWidget->setDescribeFunc(describeFunc);
        editorWidget->setParameters(parameters);
        return widget;
    });

    setEditorCreator([]() { return new VcsBaseEditor(); });
    setMarksVisible(false);
}

} // namespace VcsBase
