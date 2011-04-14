/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** This file is part of the documentation of Qt Creator.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "htmleditorplugin.h"
#include "htmleditorfactory.h"
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <QtPlugin>
#include <QString>
#include <QMessageBox>
#include <QFontDialog>

HTMLEditorPlugin::HTMLEditorPlugin()
{
    // Do nothing
}

HTMLEditorPlugin::~HTMLEditorPlugin()
{
    // Do notning
}

void HTMLEditorPlugin::extensionsInitialized()
{
    // Do nothing
}

bool HTMLEditorPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);

    Core::ActionManager* am = Core::ICore::instance()->actionManager();
    Core::ActionContainer* ac = am->actionContainer(Core::Constants::M_EDIT);
    QAction* Font = ac->menu()->addAction("Font");

    // Create a command for "Font".
    Core::Command* cmd = am->registerAction(new QAction(this),"HTMLEditorPlugin.Font",QList<int>() << 0);
    cmd->action()->setText("Font");

    Core::ICore* core = Core::ICore::instance();
    Core::MimeDatabase* mdb = core->mimeDatabase();

    if(!mdb->addMimeTypes("text-html-mimetype.xml", errMsg))
        return false;

    QMessageBox::information(0, "Msg", *errMsg);

    addAutoReleasedObject(new HTMLEditorFactory(this));
    return true;
}

void HTMLEditorPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(HTMLEditorPlugin)
