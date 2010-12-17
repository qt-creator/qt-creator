/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DESIGNMODE_H
#define DESIGNMODE_H

#include <coreplugin/imode.h>

namespace Core {
class EditorManager;
class IEditor;

namespace Internal {
class DesignModeCoreListener;
} // namespace Internal

/**
  * A global mode for Design pane - used by Bauhaus (QML Designer) and
  * Qt Designer. Other plugins can register themselves by registerDesignWidget()
  * and giving a list of mimetypes that the editor understands, as well as an instance
  * to the main editor widget itself.
  */

struct DesignModePrivate;

class CORE_EXPORT DesignMode : public Core::IMode
{
    Q_OBJECT

public:
    explicit DesignMode(EditorManager *editorManager);
    virtual ~DesignMode();

    void registerDesignWidget(QWidget *widget,
                              const QStringList &mimeTypes,
                              const Context &context);
    void unregisterDesignWidget(QWidget *widget);

    QStringList registeredMimeTypes() const;

    // IContext
    Context context() const;
    QWidget *widget();

    // IMode
    QString displayName() const;
    QIcon icon() const;
    int priority() const;
    QString id() const;
    QString type() const;

signals:
    void actionsUpdated(Core::IEditor *editor);

private slots:
    void currentEditorChanged(Core::IEditor *editor);
    void updateActions();
    void updateContext(Core::IMode *newMode, Core::IMode *oldMode);

private:
    void setActiveContext(const Context &context);

    DesignModePrivate *d;
    friend class Internal::DesignModeCoreListener;
};

} // namespace Core

#endif // DESIGNMODE_H
