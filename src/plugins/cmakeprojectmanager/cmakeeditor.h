/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

class CMakeEditorWidget;
class CMakeHighlighter;
class CMakeManager;

class CMakeEditor : public TextEditor::BaseTextEditor
{
public:
    CMakeEditor(CMakeEditorWidget *);
    Core::Context context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;
    bool isTemporary() const { return false; }
private:
    const Core::Context m_context;
};

class CMakeEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    CMakeEditorWidget(QWidget *parent, CMakeEditorFactory *factory, TextEditor::TextEditorActionHandler *ah);
    ~CMakeEditorWidget();

    bool save(const QString &fileName = QString());

    CMakeEditorFactory *factory() { return m_factory; }
    TextEditor::TextEditorActionHandler *actionHandler() const { return m_ah; }

protected:
    TextEditor::BaseTextEditor *createEditor();

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
