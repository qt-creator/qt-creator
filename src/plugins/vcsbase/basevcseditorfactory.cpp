/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basevcseditorfactory.h"
#include "vcsbaseeditor.h"

#include <texteditor/texteditoractionhandler.h>

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
                                   const EditorWidgetCreator &editorWidgetCreator,
                                   QObject *describeReceiver, const char *describeSlot)
{
    setProperty("VcsEditorFactoryName", QByteArray(parameters->id));
    setId(parameters->id);
    setDisplayName(QCoreApplication::translate("VCS", parameters->displayName));
    if (QLatin1String(parameters->mimeType) != QLatin1String(DiffEditor::Constants::DIFF_EDITOR_MIMETYPE))
        addMimeType(parameters->mimeType);

    setEditorActionHandlers(parameters->context, TextEditorActionHandler::None);

    setDocumentCreator([=]() {
        auto document = new BaseTextDocument(parameters->id);
 //  if (QLatin1String(parameters->mimeType) != QLatin1String(DiffEditor::Constants::DIFF_EDITOR_MIMETYPE))
        document->setMimeType(QLatin1String(parameters->mimeType));
        return document;
    });

    setEditorWidgetCreator([=]() {
        auto widget = qobject_cast<VcsBaseEditorWidget *>(editorWidgetCreator());
        widget->setDescribeSlot(describeReceiver, describeSlot);
        widget->setParameters(parameters);
        return widget;
    });

    setEditorCreator([=]() {
        return new VcsBaseEditor(parameters);
    });
}

VcsBaseEditor *VcsEditorFactory::createEditorById(const char *id)
{
    auto factory =  ExtensionSystem::PluginManager::getObject<VcsEditorFactory>(
        [id](QObject *ob) { return ob->property("VcsEditorFactoryName").toByteArray() == id; });
    QTC_ASSERT(factory, return 0);
    return qobject_cast<VcsBaseEditor *>(factory->createEditor());
}

} // namespace VcsBase
