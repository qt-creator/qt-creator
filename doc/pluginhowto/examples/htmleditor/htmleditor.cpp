/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "htmlfile.h"
#include "htmleditor.h"
#include "htmleditorwidget.h"

#include "coreplugin/uniqueidmanager.h"

struct HTMLEditorData
{
    HTMLEditorData() : editorWidget(0), file(0) { }

    HTMLEditorWidget* editorWidget;
    QString displayName;
    HTMLFile* file;
    QList<int> context;
};

namespace HTMLEditorConstants
{
    const char* const C_HTMLEDITOR_MIMETYPE = "text/html";
    const char* const C_HTMLEDITOR = "HTML Editor";
}

HTMLEditor::HTMLEditor(HTMLEditorWidget* editorWidget)
    :Core::IEditor(editorWidget)
{
    d = new HTMLEditorData;
    d->editorWidget = editorWidget;
    d->file = new HTMLFile(this, editorWidget);

    Core::UniqueIDManager* uidm = Core::UniqueIDManager::instance();
    d->context << uidm->uniqueIdentifier(HTMLEditorConstants::C_HTMLEDITOR);

    connect(d->editorWidget, SIGNAL(contentModified()),
            d->file, SLOT(modified()));
    connect(d->editorWidget, SIGNAL(titleChanged(QString)),
            this, SLOT(slotTitleChanged(QString)));
    connect(d->editorWidget, SIGNAL(contentModified()),
            this, SIGNAL(changed()));
}

HTMLEditor::~HTMLEditor()
{
    delete d;
}

QWidget* HTMLEditor::widget()
{
    return d->editorWidget;
}

QList<int> HTMLEditor::context() const
{
    return d->context;
}

Core::IFile* HTMLEditor::file()
{
    return d->file;
}

bool HTMLEditor::createNew(const QString& contents)
{
    Q_UNUSED(contents);

    d->editorWidget->setContent(QByteArray());
    d->file->setFilename(QString());

    return true;
}

bool HTMLEditor::open(const QString &fileName)
{
    return d->file->open(fileName);
}

const char* HTMLEditor::kind() const
{
    return HTMLEditorConstants::C_HTMLEDITOR;
}

QString HTMLEditor::displayName() const
{
    return d->displayName;
}

void HTMLEditor::setDisplayName(const QString& title)
{
    if(d->displayName == title)
        return;

    d->displayName = title;
    emit changed();
}

bool HTMLEditor::duplicateSupported() const
{
    return false;
}
Core::IEditor* HTMLEditor::duplicate(QWidget* parent)
{
    Q_UNUSED(parent);

    return 0;
}

QByteArray HTMLEditor::saveState() const
{
    return QByteArray();
}

bool HTMLEditor::restoreState(const QByteArray& state)
{
    Q_UNUSED(state);

    return false;
}

QToolBar* HTMLEditor::toolBar()
{
    return 0;
}

bool HTMLEditor::isTemporary() const
{
    return false;
}



