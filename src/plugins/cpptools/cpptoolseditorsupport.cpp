/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cpptoolseditorsupport.h"
#include "cppmodelmanager.h"

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QTimer>

using namespace CppTools::Internal;

CppEditorSupport::CppEditorSupport(CppModelManager *modelManager)
    : QObject(modelManager),
      _modelManager(modelManager),
      _updateDocumentInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL)
{
    _updateDocumentTimer = new QTimer(this);
    _updateDocumentTimer->setSingleShot(true);
    _updateDocumentTimer->setInterval(_updateDocumentInterval);
    connect(_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));
}

CppEditorSupport::~CppEditorSupport()
{ }

TextEditor::ITextEditor *CppEditorSupport::textEditor() const
{ return _textEditor; }

void CppEditorSupport::setTextEditor(TextEditor::ITextEditor *textEditor)
{
    _textEditor = textEditor;

    if (! _textEditor)
        return;

    connect(_textEditor, SIGNAL(contentsChanged()), this, SLOT(updateDocument()));
    updateDocument();
}

QString CppEditorSupport::contents()
{
    if (! _textEditor)
        return QString();
    else if (! _cachedContents.isEmpty())
        _cachedContents = _textEditor->contents();

    return _cachedContents;
}

int CppEditorSupport::updateDocumentInterval() const
{ return _updateDocumentInterval; }

void CppEditorSupport::setUpdateDocumentInterval(int updateDocumentInterval)
{ _updateDocumentInterval = updateDocumentInterval; }

void CppEditorSupport::updateDocument()
{
    if (TextEditor::BaseTextEditor *edit = qobject_cast<TextEditor::BaseTextEditor*>(_textEditor->widget())) {
        const QList<QTextEdit::ExtraSelection> selections =
                edit->extraSelections(TextEditor::BaseTextEditor::CodeWarningsSelection);

        _modelManager->stopEditorSelectionsUpdate();
    }

    _updateDocumentTimer->start(_updateDocumentInterval);
}

void CppEditorSupport::updateDocumentNow()
{
    if (_documentParser.isRunning()) {
        _updateDocumentTimer->start(_updateDocumentInterval);
    } else {
        _updateDocumentTimer->stop();

        QStringList sourceFiles(_textEditor->file()->fileName());
        _cachedContents = _textEditor->contents();
        _documentParser = _modelManager->refreshSourceFiles(sourceFiles);
    }
}

