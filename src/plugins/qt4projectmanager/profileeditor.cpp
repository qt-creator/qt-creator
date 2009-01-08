/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "profileeditor.h"

#include "profilehighlighter.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "profileeditorfactory.h"
#include "proeditormodel.h"
#include "procommandmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QtCore/QFileInfo>
#include <QtGui/QTextEdit>
#include <QtGui/QHeaderView>
#include <QtCore/QDebug>

using namespace ExtensionSystem;
using namespace Core;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;


ProFileEditorEditable::ProFileEditorEditable(ProFileEditor *editor, Core::ICore *core)
    :BaseTextEditorEditable(editor)
{
    m_context << core->uniqueIDManager()->
        uniqueIdentifier(Qt4ProjectManager::Constants::C_PROFILEEDITOR);
    m_context << core->uniqueIDManager()->
        uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
//    m_contexts << core->uniqueIDManager()->
//        uniqueIdentifier(Qt4ProjectManager::Constants::PROJECT_KIND);
}

TextEditor::BaseTextEditorEditable *ProFileEditor::createEditableInterface()
{
    return new ProFileEditorEditable(this, m_core);
}

ProFileEditor::ProFileEditor(QWidget *parent, ProFileEditorFactory *factory, TextEditor::TextEditorActionHandler *ah)
    : BaseTextEditor(parent), m_factory(factory), m_ah(ah)
{
    Qt4Manager *manager = factory->qt4ProjectManager();
    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();

    ProFileDocument *doc = new ProFileDocument(manager);
    doc->setMimeType(QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE));
    setBaseTextDocument(doc);

    ah->setupActions(this);

    baseTextDocument()->setSyntaxHighlighter(new ProFileHighlighter);
}

ProFileEditor::~ProFileEditor()
{
}

QList<int> ProFileEditorEditable::context() const
{
    return m_context;
}

Core::IEditor *ProFileEditorEditable::duplicate(QWidget *parent)
{
    ProFileEditor *ret = new ProFileEditor(parent, qobject_cast<ProFileEditor*>(editor())->factory(),
                                           qobject_cast<ProFileEditor*>(editor())->actionHandler());
    ret->duplicateFrom(editor());
    ret->initialize();
    return ret->editableInterface();
}

void ProFileEditor::initialize()
{
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();

    connect(settings, SIGNAL(fontSettingsChanged(const TextEditor::FontSettings&)),
            this, SLOT(setFontSettings(const TextEditor::FontSettings&)));

    setFontSettings(settings->fontSettings());
}

const char *ProFileEditorEditable::kind() const
{
    return Qt4ProjectManager::Constants::PROFILE_EDITOR;
}

void ProFileEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    ProFileHighlighter *highlighter = qobject_cast<ProFileHighlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_TYPE)
                   << QLatin1String(TextEditor::Constants::C_KEYWORD)
                   << QLatin1String(TextEditor::Constants::C_COMMENT);
    }

    const QVector<QTextCharFormat> formats = fs.toTextCharFormats(categories);
    highlighter->setFormats(formats.constBegin(), formats.constEnd());
    highlighter->rehighlight();
}

ProFileDocument::ProFileDocument(Qt4Manager *manager)
        : TextEditor::BaseTextDocument(), m_manager(manager)
{
}

bool ProFileDocument::save(const QString &name)
{
    if (BaseTextDocument::save(name)) {
        m_manager->notifyChanged(name);
        return true;
    }
    return false;
}

QString ProFileDocument::defaultPath() const
{
    QFileInfo fi(fileName());
    return fi.absolutePath();
}

QString ProFileDocument::suggestedFileName() const
{
    QFileInfo fi(fileName());
    return fi.fileName();
}
