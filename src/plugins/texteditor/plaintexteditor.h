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

#ifndef PLAINTEXTEDITOR_H
#define PLAINTEXTEDITOR_H

#include "basetexteditor.h"

#include <utils/uncommentselection.h>

#include <QtCore/QList>
#include <QtCore/QScopedPointer>

namespace Core {
class MimeType;
}

namespace TextEditor {

class PlainTextEditor;
class Indenter;

class TEXTEDITOR_EXPORT PlainTextEditorEditable : public BaseTextEditorEditable
{
    Q_OBJECT
public:
    PlainTextEditorEditable(PlainTextEditor *);
    Core::Context context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    bool isTemporary() const { return false; }
    virtual QString id() const;

private:
    const Core::Context m_context;
};

class TEXTEDITOR_EXPORT PlainTextEditor : public BaseTextEditor
{
    Q_OBJECT

public:
    PlainTextEditor(QWidget *parent);
    ~PlainTextEditor();

    void configure(const Core::MimeType &mimeType);
    bool isMissingSyntaxDefinition() const;

public slots:
    virtual void unCommentSelection();
    virtual void setFontSettings(const FontSettings &fs);
    virtual void setTabSettings(const TextEditor::TabSettings &);

private slots:
    void fileChanged();

protected:
    virtual BaseTextEditorEditable *createEditableInterface() { return new PlainTextEditorEditable(this); }    
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);

private:
    QString findDefinitionId(const Core::MimeType &mimeType, bool considerParents) const;

    bool m_isMissingSyntaxDefinition;
    Utils::CommentDefinition m_commentDefinition;
    QScopedPointer<Indenter> m_indenter;
};

} // namespace TextEditor

#endif // PLAINTEXTEDITOR_H
