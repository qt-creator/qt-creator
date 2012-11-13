/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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

public slots:
    virtual void unCommentSelection();
    virtual void setFontSettings(const FontSettings &fs);
    virtual void setTabSettings(const TextEditor::TabSettings &);

private slots:
    void configure();
    void acceptMissingSyntaxDefinitionInfo();

signals:
    void configured(Core::IEditor *editor);

protected:
    virtual BaseTextEditor *createEditor() { return new PlainTextEditor(this); }

private:
    QString findDefinitionId(const Core::MimeType &mimeType, bool considerParents) const;

    bool m_isMissingSyntaxDefinition;
    Utils::CommentDefinition m_commentDefinition;
};

} // namespace TextEditor

#endif // PLAINTEXTEDITOR_H
