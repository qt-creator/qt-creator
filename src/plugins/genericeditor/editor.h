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

#ifndef GENERICEDITOR_H
#define GENERICEDITOR_H

#include <texteditor/basetexteditor.h>
#include <utils/uncommentselection.h>

#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace GenericEditor {
namespace Internal {

class Editor : public TextEditor::BaseTextEditor
{
    Q_OBJECT
public:
    Editor(QWidget *parent = 0);

    virtual void unCommentSelection();

protected:
    virtual TextEditor::BaseTextEditorEditable *createEditableInterface();

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

private slots:
    void configure();

private:
    Utils::CommentDefinition m_commentDefinition;
};

class EditorEditable : public TextEditor::BaseTextEditorEditable
{
    Q_OBJECT
public:
    EditorEditable(Editor *editor);

protected:
    virtual QString id() const;
    virtual QList<int> context() const;
    virtual bool isTemporary() const;
    virtual bool duplicateSupported() const;
    virtual Core::IEditor *duplicate(QWidget *parent);

private:
    QList<int> m_context;
};



} // namespace Internal
} // namespace GenericEditor

#endif // GENERICEDITOR_H
