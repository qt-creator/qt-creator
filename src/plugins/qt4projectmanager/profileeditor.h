/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef PROFILEEDITOR_H
#define PROFILEEDITOR_H

#include "ui_proeditorcontainer.h"

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>

namespace TextEditor {
class FontSettings;
class BaseEditorActionHandler;
}

namespace Core {
class ICore;
class IFile;
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
    ProFileEditorEditable(ProFileEditor *, Core::ICore *core);
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
    Core::ICore *m_core;
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
