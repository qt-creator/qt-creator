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

#ifndef FORMWINDOWEDITOR_H
#define FORMWINDOWEDITOR_H

#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QDesignerFormWindowManagerInterface;
class QFile;
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

// Master class maintaining a form window editor,
// containing file and widget host

class FormWindowEditor : public Core::IEditor
{
    Q_OBJECT
public:
    FormWindowEditor(const QList<int> &context,
                     QDesignerFormWindowInterface *form,
                     QObject *parent = 0);
    ~FormWindowEditor();

    // IEditor
    bool createNew(const QString &contents);
    bool open(const QString &fileName = QString());
    bool duplicateSupported() const;
    Core::IEditor *duplicate(QWidget *);
    Core::IFile *file();
    const char *kind() const;
    QString displayName() const;
    void setDisplayName(const QString &title);
    QToolBar *toolBar();
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    // ContextInterface
    virtual QList<int> context() const;
    virtual QWidget *widget();
    virtual QString contextHelpId() const;

    QDesignerFormWindowInterface *formWindow() const;
    QWidget *integrationContainer();
    void updateFormWindowSelectionHandles(bool state);
    void setSuggestedFileName(const QString &fileName);

signals:
    // Internal
    void opened(const QString &fileName);

public slots:
    void activate();

private slots:
    void slotOpen(const QString &fileName);
    void slotSetDisplayName(const QString &title);
    void updateResources();

private:
    QString m_displayName;
    const QList<int> m_context;
    QDesignerFormWindowInterface *m_formWindow;
    FormWindowFile *m_file;
    FormWindowHost *m_host;
    EditorWidget *m_editorWidget;
    QToolBar *m_toolBar;
    QStringList m_originalUiQrcPaths;
    ProjectExplorer::SessionNode *m_sessionNode;
    ProjectExplorer::NodesWatcher *m_sessionWatcher;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWINDOWEDITOR_H
