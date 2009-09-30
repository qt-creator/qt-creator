/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmleditorplugin.h"

#include "qscripthighlighter.h"
#include "qmleditor.h"
#include "qmleditorconstants.h"
#include "qmleditorfactory.h"
#include "qmlcodecompletion.h"
#include "qmlhoverhandler.h"
#include "qmlmodelmanager.h"
#include "qmlfilewizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/completionsupport.h>
#include <utils/qtcassert.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QAction>

using namespace QmlEditor;
using namespace QmlEditor::Internal;
using namespace QmlEditor::Constants;

QmlEditorPlugin *QmlEditorPlugin::m_instance = 0;

QmlEditorPlugin::QmlEditorPlugin() :
        m_modelManager(0),
    m_wizard(0),
    m_editor(0),
    m_actionHandler(0),
    m_completion(0)
{
    m_instance = this;
}

QmlEditorPlugin::~QmlEditorPlugin()
{
    removeObject(m_editor);
    delete m_actionHandler;
    m_instance = 0;
}

bool QmlEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    typedef SharedTools::QScriptHighlighter QScriptHighlighter;

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/qmleditor/QmlEditor.mimetypes.xml"), error_message))
        return false;

    m_modelManager = new QmlModelManager(this);
    addAutoReleasedObject(m_modelManager);

    m_scriptcontext << core->uniqueIDManager()->uniqueIdentifier(QmlEditor::Constants::C_QMLEDITOR);
    m_context = m_scriptcontext;
    m_context << core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);

    registerActions();

    m_editor = new QmlEditorFactory(m_context, this);
    addObject(m_editor);

    Core::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setCategory(QLatin1String("Qt"));
    wizardParameters.setTrCategory(tr("Qt"));
    wizardParameters.setDescription(tr("Creates a Qt QML file."));
    wizardParameters.setName(tr("Qt QML File"));
    addAutoReleasedObject(new QmlFileWizard(wizardParameters, core));

    m_actionHandler = new TextEditor::TextEditorActionHandler(QmlEditor::Constants::C_QMLEDITOR,
          TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll);
    m_actionHandler->initializeActions();

    m_completion = new QmlCodeCompletion();
    addAutoReleasedObject(m_completion);

    addAutoReleasedObject(new QmlHoverHandler());

    // Restore settings
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("CppTools")); // ### FIXME:
    settings->beginGroup(QLatin1String("Completion"));
    const bool caseSensitive = settings->value(QLatin1String("CaseSensitive"), true).toBool();
    m_completion->setCaseSensitivity(caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    settings->endGroup();
    settings->endGroup();
    
    error_message->clear();

    return true;
}

void QmlEditorPlugin::extensionsInitialized()
{
}

void QmlEditorPlugin::initializeEditor(QmlEditor::Internal::ScriptEditor *editor)
{
    QTC_ASSERT(m_instance, /**/);

    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(ITextEditable*, bool)),
            TextEditor::Internal::CompletionSupport::instance(), SLOT(autoComplete(ITextEditable*, bool)));
}

void QmlEditorPlugin::registerActions()
{
}

Q_EXPORT_PLUGIN(QmlEditorPlugin)
