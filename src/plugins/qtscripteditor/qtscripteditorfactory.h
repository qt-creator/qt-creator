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

#ifndef RQTSCRIPTEDITORFACTORY_H
#define RQTSCRIPTEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QStringList>

namespace TextEditor {
class TextEditorActionHandler;
}

namespace QtScriptEditor {
namespace Internal {

class QtScriptEditorActionHandler;

class QtScriptEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    typedef QList<int> Context;

    QtScriptEditorFactory(const Context &context, QObject *parent);
    ~QtScriptEditorFactory();

    virtual QStringList mimeTypes() const;
    // IEditorFactory
    QString kind() const;
    Core::IFile *open(const QString &fileName);
    Core::IEditor *createEditor(QWidget *parent);

private:
    const QString m_kind;
    const QStringList m_mimeTypes;
    const Context m_context;

    TextEditor::TextEditorActionHandler *m_actionHandler;
};

} // namespace Internal
} // namespace QtScriptEditor

#endif // RQTSCRIPTEDITORFACTORY_H
