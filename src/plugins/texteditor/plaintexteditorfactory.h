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

#ifndef PLAINTEXTEDITORFACTORY_H
#define PLAINTEXTEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>
#include <QtCore/QStringList>

namespace Core {
class IEditor;
class IFile;
}

namespace TextEditor {
class  TextEditorActionHandler;
namespace Internal {

class PlainTextEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    PlainTextEditorFactory(QObject *parent = 0);
    virtual ~PlainTextEditorFactory();

    virtual QStringList mimeTypes() const;
    //Core::IEditorFactory
    QString kind() const;
    Core::IFile *open(const QString &fileName);
    Core::IEditor *createEditor(QWidget *parent);

    TextEditor::TextEditorActionHandler *actionHandler() const { return m_actionHandler; }

private:
    const QString m_kind;
    QStringList m_mimeTypes;
    TextEditor::TextEditorActionHandler *m_actionHandler;
};

} // namespace Internal
} // namespace TextEditor

#endif // PLAINTEXTEDITORFACTORY_H
