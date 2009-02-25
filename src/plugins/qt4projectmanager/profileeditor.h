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

#ifndef PROFILEEDITOR_H
#define PROFILEEDITOR_H

#include "ui_proeditorcontainer.h"

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>

namespace TextEditor {
class FontSettings;
}

namespace Qt4ProjectManager {

class Qt4Manager;
class Qt4Project;

namespace Internal {

class ProFileEditorFactory;
class ProItemInfoManager;
class ProEditorModel;
class ProFileHighlighter;

class ProFileEditor;

class ProFileEditorEditable : public TextEditor::BaseTextEditorEditable
{
public:
    ProFileEditorEditable(ProFileEditor *);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    const char *kind() const;
private:
    QList<int> m_context;
};

class ProFileEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    ProFileEditor(QWidget *parent, ProFileEditorFactory *factory,
                  TextEditor::TextEditorActionHandler *ah);
    ~ProFileEditor();
    void initialize();

    bool save(const QString &fileName = QString());


    ProFileEditorFactory *factory() { return m_factory; }
    TextEditor::TextEditorActionHandler *actionHandler() const { return m_ah; }
protected:
    TextEditor::BaseTextEditorEditable *createEditableInterface();

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

private:
    ProFileEditorFactory *m_factory;
    TextEditor::TextEditorActionHandler *m_ah;
};

class ProFileDocument : public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    ProFileDocument(Qt4Manager *manager);
    bool save(const QString &name);
    QString defaultPath() const;
    QString suggestedFileName() const;

private:
    Qt4Manager *m_manager;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEEDITOR_H
