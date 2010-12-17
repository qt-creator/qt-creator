/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    virtual BaseTextEditorEditable *createEditableInterface() { return new PlainTextEditorEditable(this); }    

private:
    QString findDefinitionId(const Core::MimeType &mimeType, bool considerParents) const;

    bool m_isMissingSyntaxDefinition;
    bool m_ignoreMissingSyntaxDefinition;
    Utils::CommentDefinition m_commentDefinition;
};

} // namespace TextEditor

#endif // PLAINTEXTEDITOR_H
