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

#ifndef CMAKEEDITOR_H
#define CMAKEEDITOR_H

#include "cmakeeditorfactory.h"

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>


namespace TextEditor {
class FontSettings;
}

namespace CMakeProjectManager {

namespace Internal {

class CMakeManager;
class CMakeHighlighter;

class CMakeEditor;

class CMakeEditorEditable : public TextEditor::BaseTextEditorEditable
{
public:
    CMakeEditorEditable(CMakeEditor *);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;
    bool isTemporary() const { return false; }
private:
    QList<int> m_context;
};

class CMakeEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    CMakeEditor(QWidget *parent, CMakeEditorFactory *factory, TextEditor::TextEditorActionHandler *ah);
    ~CMakeEditor();

    bool save(const QString &fileName = QString());

    CMakeEditorFactory *factory() { return m_factory; }
    TextEditor::TextEditorActionHandler *actionHandler() const { return m_ah; }
protected:
    TextEditor::BaseTextEditorEditable *createEditableInterface();

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

private:
    CMakeEditorFactory *m_factory;
    TextEditor::TextEditorActionHandler *m_ah;
};

class CMakeDocument : public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    CMakeDocument();
    QString defaultPath() const;
    QString suggestedFileName() const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // CMAKEEDITOR_H
