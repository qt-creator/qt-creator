/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PLAINTEXTEDITORFACTORY_H
#define PLAINTEXTEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QStringList>

namespace TextEditor {
class  TextEditorActionHandler;
namespace Internal {

class PlainTextEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    PlainTextEditorFactory(QObject *parent = 0);
    virtual ~PlainTextEditorFactory();

    void addMimeType(const QString &type);
    virtual QStringList mimeTypes() const;
    //Core::IEditorFactory
    Core::Id id() const;
    QString displayName() const;
    Core::IEditor *createEditor(QWidget *parent);

    TextEditor::TextEditorActionHandler *actionHandler() const { return m_actionHandler; }

private slots:
    void updateEditorInfoBar(Core::IEditor *editor);

private:
    QStringList m_mimeTypes;
    TextEditor::TextEditorActionHandler *m_actionHandler;
};

} // namespace Internal
} // namespace TextEditor

#endif // PLAINTEXTEDITORFACTORY_H
