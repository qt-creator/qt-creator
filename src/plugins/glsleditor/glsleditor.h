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

#ifndef GLSLEDITOR_H
#define GLSLEDITOR_H

#include "glsleditor_global.h"
#include "glsleditoreditable.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/quickfix.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QSet>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace GLSLEditor {

class GLSLEDITOR_EXPORT GLSLTextEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    GLSLTextEditor(QWidget *parent = 0);
    ~GLSLTextEditor();

    virtual void unCommentSelection();

    int editorRevision() const;
    bool isOutdated() const;

    QSet<QString> identifiers() const;

    bool isVertexShader() const;
    bool isFragmentShader() const;

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

private slots:
    void updateDocument();
    void updateDocumentNow();

protected:
    bool event(QEvent *e);
    TextEditor::BaseTextEditorEditable *createEditableInterface();
    void createToolBar(Internal::GLSLEditorEditable *editable);

private:
    void setSelectedElements();
    QString wordUnderCursor() const;

    const Core::Context m_context;

    QTimer *m_updateDocumentTimer;
    QComboBox *m_outlineCombo;
    QSet<QString> m_identifiers;
};

} // namespace GLSLEditor

#endif // GLSLEDITOR_H
