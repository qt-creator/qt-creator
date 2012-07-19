/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PLAINTEXTEDITOR_H
#define PLAINTEXTEDITOR_H

#include "basetexteditor.h"

#include <utils/uncommentselection.h>

namespace Core {
class MimeType;
}

namespace TextEditor {

class PlainTextEditorWidget;
class Indenter;

class TEXTEDITOR_EXPORT PlainTextEditor : public BaseTextEditor
{
    Q_OBJECT
public:
    PlainTextEditor(PlainTextEditorWidget *);

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    bool isTemporary() const { return false; }
    Core::Id id() const;
};

class TEXTEDITOR_EXPORT PlainTextEditorWidget : public BaseTextEditorWidget
{
    Q_OBJECT

public:
    PlainTextEditorWidget(QWidget *parent);

    void configure(const QString& mimeType);
    void configure(const Core::MimeType &mimeType);
    bool isMissingSyntaxDefinition() const;
    bool ignoreMissingSyntaxDefinition() const;

public slots:
    virtual void unCommentSelection();
    virtual void setFontSettings(const FontSettings &fs);
    virtual void setTabSettings(const TextEditor::TabSettings &);

private slots:
    void configure();
    void acceptMissingSyntaxDefinitionInfo();
    void ignoreMissingSyntaxDefinitionInfo();

signals:
    void configured(Core::IEditor *editor);

protected:
    virtual BaseTextEditor *createEditor() { return new PlainTextEditor(this); }

private:
    QString findDefinitionId(const Core::MimeType &mimeType, bool considerParents) const;

    bool m_isMissingSyntaxDefinition;
    bool m_ignoreMissingSyntaxDefinition;
    Utils::CommentDefinition m_commentDefinition;
};

} // namespace TextEditor

#endif // PLAINTEXTEDITOR_H
