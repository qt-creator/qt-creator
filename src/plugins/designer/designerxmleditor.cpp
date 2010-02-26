/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designerxmleditor.h"
#include "designerconstants.h"
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <QDebug>

using namespace Designer::Internal;

DesignerXmlEditor::DesignerXmlEditor(QWidget *parent) : TextEditor::PlainTextEditor(parent)
{
    setReadOnly(true);
    connect(Core::ICore::instance()->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            SLOT(updateEditorInfoBar(Core::IEditor*)));
}

DesignerXmlEditor::~DesignerXmlEditor()
{

}

bool DesignerXmlEditor::open(const QString &fileName)
{
    bool res = TextEditor::PlainTextEditor::open(fileName);
    QPlainTextEdit::setReadOnly(true);
    return res;
}

void DesignerXmlEditor::updateEditorInfoBar(Core::IEditor *editor)
{
    if (editor == editableInterface()) {
        Core::EditorManager::instance()->showEditorInfoBar(Constants::INFO_READ_ONLY,
            tr("This file can only be edited in Design Mode."),
            "Open Designer", this, SLOT(designerOpened()));
    }
    if (!editor)
        Core::EditorManager::instance()->hideEditorInfoBar(Constants::INFO_READ_ONLY);
}

void DesignerXmlEditor::designerOpened()
{
    Core::ICore::instance()->modeManager()->activateMode(Core::Constants::MODE_DESIGN);
}

QString DesignerXmlEditorEditable::id() const
{
    return QLatin1String(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
}
DesignerXmlEditorEditable::DesignerXmlEditorEditable(DesignerXmlEditor *editor)
    : TextEditor::PlainTextEditorEditable(editor)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
    m_context << uidm->uniqueIdentifier(Designer::Constants::C_DESIGNER_XML_EDITOR);
}

QList<int> DesignerXmlEditorEditable::context() const
{
    return m_context;
}

Core::IEditor *DesignerXmlEditorEditable::duplicate(QWidget *parent)
{
    Q_UNUSED(parent);
    return 0;
}
