/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "diffeditorplugin.h"
#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditorfactory.h"
#include "diffeditorwidget.h"
#include "diffshoweditor.h"
#include "diffshoweditorfactory.h"

#include <QFileDialog>
#include <QTextCodec>
#include <QtPlugin>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

namespace DiffEditor {

namespace Internal {

DiffEditorPlugin::DiffEditorPlugin()
{
}

DiffEditorPlugin::~DiffEditorPlugin()
{
}

bool DiffEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    //register actions
    Core::ActionContainer *toolsContainer =
        Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_OPTIONS, Constants::G_TOOLS_DIFF);

    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    QAction *diffAction = new QAction(tr("Diff..."), this);
    Core::Command *diffCommand = Core::ActionManager::registerAction(diffAction,
                             "DiffEditor.Diff", globalcontext);
    connect(diffAction, SIGNAL(triggered()), this, SLOT(diff()));
    toolsContainer->addAction(diffCommand, Constants::G_TOOLS_DIFF);

    addAutoReleasedObject(new DiffEditorFactory(this));
    addAutoReleasedObject(new DiffShowEditorFactory(this));

    return true;
}

void DiffEditorPlugin::extensionsInitialized()
{
}

void DiffEditorPlugin::diff()
{
    QString fileName1 = QFileDialog::getOpenFileName(0,
                                                     tr("Select First File for Diff"),
                                                     QString());
    if (fileName1.isNull())
        return;

    QString fileName2 = QFileDialog::getOpenFileName(0,
                                                     tr("Select Second File for Diff"),
                                                     QString());
    if (fileName2.isNull())
        return;


    const Core::Id editorId = Constants::DIFF_EDITOR_ID;
    //: Editor title
    QString title = tr("Diff \"%1\", \"%2\"").arg(fileName1).arg(fileName2);
    DiffEditor *editor = qobject_cast<DiffEditor *>
            (Core::EditorManager::openEditorWithContents(editorId, &title, QString()));

    if (!editor)
        return;

    Core::EditorManager::activateEditor(editor);

    DiffEditorWidget *editorWidget = editor->editorWidget();

    const QString text1 = getFileContents(fileName1, editorWidget->codec());
    const QString text2 = getFileContents(fileName2, editorWidget->codec());

    DiffEditorWidget::DiffFilesContents dfc;
    dfc.leftFileInfo = fileName1;
    dfc.leftText = text1;
    dfc.rightFileInfo = fileName2;
    dfc.rightText = text2;
    QList<DiffEditorWidget::DiffFilesContents> list;
    list.append(dfc);

    editor->setDiff(list);
}

QString DiffEditorPlugin::getFileContents(const QString &fileName, QTextCodec *codec) const
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return codec->toUnicode(file.readAll());
    }
    return QString();
}

} // namespace Internal
} // namespace DiffEditor

#ifdef WITH_TESTS

void DiffEditor::Internal::DiffEditorPlugin::testAssemblyRows()
{
    DiffEditorWidget widget;
    widget.testAssemblyRows();
}

#endif // WITH_TESTS

Q_EXPORT_PLUGIN(DiffEditor::Internal::DiffEditorPlugin)
