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

#include "cmakeeditor.h"

#include "cmakehighlighter.h"
#include "cmakeeditorfactory.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/uniqueidmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QtCore/QFileInfo>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

//
// ProFileEditorEditable
//

CMakeEditorEditable::CMakeEditorEditable(CMakeEditor *editor)
    : BaseTextEditorEditable(editor)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(CMakeProjectManager::Constants::C_CMAKEEDITOR);
    m_context << uidm->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
}

QList<int> CMakeEditorEditable::context() const
{
    return m_context;
}

Core::IEditor *CMakeEditorEditable::duplicate(QWidget *parent)
{
    CMakeEditor *ret = new CMakeEditor(parent, qobject_cast<CMakeEditor*>(editor())->factory(),
                                       qobject_cast<CMakeEditor*>(editor())->actionHandler());
    ret->duplicateFrom(editor());
    TextEditor::TextEditorSettings::instance()->initializeEditor(ret);
    return ret->editableInterface();
}

QString CMakeEditorEditable::id() const
{
    return QLatin1String(CMakeProjectManager::Constants::CMAKE_EDITOR_ID);
}

//
// CMakeEditor
//

CMakeEditor::CMakeEditor(QWidget *parent, CMakeEditorFactory *factory, TextEditor::TextEditorActionHandler *ah)
    : BaseTextEditor(parent), m_factory(factory), m_ah(ah)
{
    CMakeDocument *doc = new CMakeDocument();
    doc->setMimeType(QLatin1String(CMakeProjectManager::Constants::CMAKEMIMETYPE));
    setBaseTextDocument(doc);

    ah->setupActions(this);

    baseTextDocument()->setSyntaxHighlighter(new CMakeHighlighter);
}

CMakeEditor::~CMakeEditor()
{
}

TextEditor::BaseTextEditorEditable *CMakeEditor::createEditableInterface()
{
    return new CMakeEditorEditable(this);
}

void CMakeEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    CMakeHighlighter *highlighter = qobject_cast<CMakeHighlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_LABEL)  // variables
                << QLatin1String(TextEditor::Constants::C_LINK)   // functions
                << QLatin1String(TextEditor::Constants::C_COMMENT)
                << QLatin1String(TextEditor::Constants::C_STRING);
    }

    const QVector<QTextCharFormat> formats = fs.toTextCharFormats(categories);
    highlighter->setFormats(formats.constBegin(), formats.constEnd());
    highlighter->rehighlight();
}

//
// ProFileDocument
//

CMakeDocument::CMakeDocument()
    : TextEditor::BaseTextDocument()
{
}

QString CMakeDocument::defaultPath() const
{
    QFileInfo fi(fileName());
    return fi.absolutePath();
}

QString CMakeDocument::suggestedFileName() const
{
    QFileInfo fi(fileName());
    return fi.fileName();
}
