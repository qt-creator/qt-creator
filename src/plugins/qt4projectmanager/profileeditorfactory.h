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

#ifndef PROFILEEDITORFACTORY_H
#define PROFILEEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QStringList>

namespace TextEditor {
class TextEditorActionHandler;
}

namespace Qt4ProjectManager {

class Qt4Manager;

namespace Internal {

class ProFileEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    ProFileEditorFactory(Qt4Manager *parent, TextEditor::TextEditorActionHandler *handler);
    ~ProFileEditorFactory();

    // IEditorFactory
    QStringList mimeTypes() const;
    Core::Id id() const;
    QString displayName() const;
    Core::IEditor *createEditor(QWidget *parent);

    Qt4Manager *qt4ProjectManager() const { return m_manager; }

private:
    const QStringList m_mimeTypes;
    Qt4Manager *m_manager;
    TextEditor::TextEditorActionHandler *m_actionHandler;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEEDITORFACTORY_H
