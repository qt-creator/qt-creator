/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DESIGNMODE_H
#define DESIGNMODE_H

#include <coreplugin/imode.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QWeakPointer>

class QAction;

namespace QmlDesigner {
namespace Internal {

class DesignMode;
class DesignModeWidget;

class DesignModeCoreListener : public Core::ICoreListener
{
    Q_OBJECT
public:
    DesignModeCoreListener(DesignMode* mode);
    bool coreAboutToClose();
private:
    DesignMode *m_mode;
};

class DesignMode : public Core::IMode
{
    Q_OBJECT

public:
    DesignMode();
    ~DesignMode();

    // IContext
    QList<int> context() const;
    QWidget *widget();

    // IMode
    QString displayName() const;
    QIcon icon() const;
    int priority() const;
    QString id() const;

private slots:
    void textEditorsClosed(QList<Core::IEditor *> editors);
    void modeChanged(Core::IMode *mode);
    void currentEditorChanged(Core::IEditor *editor);
    void makeCurrentEditorWritable();
    void updateActions();

private:
    DesignModeWidget *m_mainWidget;
    DesignModeCoreListener *m_coreListener;
    QWeakPointer<Core::IEditor> m_currentEditor;
    bool m_isActive;

    QAction *m_revertToSavedAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeCurrentEditorAction;
    QAction *m_closeAllEditorsAction;
    QAction *m_closeOtherEditorsAction;

    friend class DesignModeCoreListener;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // DESIGNMODE_H
