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

#ifndef GLSLEDITOREDITABLE_H
#define GLSLEDITOREDITABLE_H

#include <texteditor/basetexteditor.h>

namespace GLSLEditor {
class GLSLTextEditorWidget;

namespace Internal {

class GLSLEditorEditable : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    explicit GLSLEditorEditable(GLSLTextEditorWidget *);
    Core::Context context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    QString id() const;
    bool isTemporary() const { return false; }
    virtual bool open(const QString & fileName);
    virtual QString preferredModeType() const;

private:
    Core::Context m_context;
};

} // namespace Internal
} // namespace GLSLEditor

#endif // GLSLEDITOREDITABLE_H
