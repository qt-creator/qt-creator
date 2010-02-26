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

#ifndef FORMWINDOWEDITOR_H
#define FORMWINDOWEDITOR_H

#include "designer_export.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QStringList>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QDesignerFormWindowManagerInterface;
class QFile;
class QToolBar;
class QDockWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class SessionNode;
class NodesWatcher;
}

namespace Designer {
namespace Internal {
class FormWindowFile;
class FormWindowHost;
class EditorWidget;
class FakeToolBar;
}

// Master class maintaining a form window editor,
// containing file and widget host

class DESIGNER_EXPORT FormWindowEditor : public Core::IEditor
{
    Q_OBJECT
public:
    FormWindowEditor(QDesignerFormWindowInterface *form,
                     QObject *parent = 0);
    ~FormWindowEditor();

    // IEditor
    bool createNew(const QString &contents);
    bool open(const QString &fileName = QString());
    bool duplicateSupported() const;
    Core::IEditor *duplicate(QWidget *);
    Core::IFile *file();
    QString id() const;
    QString displayName() const;
    void setDisplayName(const QString &title);
    QWidget *toolBar();
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    virtual bool isTemporary() const { return false; }

    void setContext(QList<int> ctx);
    // ContextInterface
    virtual QList<int> context() const;
    virtual QWidget *widget();
    virtual QString contextHelpId() const;

    QDesignerFormWindowInterface *formWindow() const;
    QWidget *integrationContainer();
    void updateFormWindowSelectionHandles(bool state);
    QDockWidget* const* dockWidgets() const;
    bool isLocked() const;
    void setLocked(bool locked);

    QString contents() const;\
    void setFile(Core::IFile *file);

signals:
    // Internal
    void opened(const QString &fileName);

public slots:
    void activate();
    void resetToDefaultLayout();

private slots:
    void slotOpen(const QString &fileName);
    void slotSetDisplayName(const QString &title);
    void updateResources();

private:
    QWidget *m_containerWidget;
    QString m_displayName;
    QList<int> m_context;
    QDesignerFormWindowInterface *m_formWindow;
    QPointer<Core::IFile> m_file;
    Internal::FormWindowHost *m_host;
    Internal::EditorWidget *m_editorWidget;
    QToolBar *m_toolBar;
    QStringList m_originalUiQrcPaths;
    ProjectExplorer::SessionNode *m_sessionNode;
    ProjectExplorer::NodesWatcher *m_sessionWatcher;
    Internal::FakeToolBar *m_fakeToolBar;
};

} // namespace Designer

#endif // FORMWINDOWEDITOR_H
