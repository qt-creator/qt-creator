/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef UICODEMODELSUPPORT_H
#define UICODEMODELSUPPORT_H

#include "qtsupport_global.h"

#include <cpptools/abstracteditorsupport.h>

#include <QDateTime>
#include <QHash>
#include <QProcess>
#include <QSet>

namespace Core { class IEditor; }
namespace CPlusPlus { class CppModelManager; }
namespace ProjectExplorer { class Project; }

namespace QtSupport {

namespace Internal { class QtSupportPlugin; }

class UiCodeModelSupport : public CppTools::AbstractEditorSupport
{
    Q_OBJECT

public:
    UiCodeModelSupport(CppTools::CppModelManager *modelmanager,
                       ProjectExplorer::Project *project,
                       const QString &sourceFile,
                       const QString &uiHeaderFile);
    ~UiCodeModelSupport();

    void setHeaderFileName(const QString &name);
    /// \returns the contents encoded in UTF-8.
    QByteArray contents() const;
    QString uiFileName() const; // The .ui-file
    QString fileName() const; // The header file
    QString headerFileName() const { return fileName(); }
    void updateFromEditor(const QString &formEditorContents);
    void updateFromBuild();

private:
    QString uicCommand() const;
    QStringList environment() const;

private slots:
    bool finishProcess();

private:
    ProjectExplorer::Project *m_project;
    enum State { BARE, RUNNING, FINISHED, ABORTING };

    void init() const;
    bool runUic(const QString &ui) const;
    mutable QProcess m_process;
    QString m_uiFileName;
    QString m_headerFileName;
    mutable State m_state;
    mutable QByteArray m_contents;
    mutable QDateTime m_cacheTime;
    static QList<UiCodeModelSupport *> m_waitingForStart;
};

class QTSUPPORT_EXPORT UiCodeModelManager : public QObject
{
    Q_OBJECT

public:
    // This needs to be called by the project *before* the C++ code model is updated!
    static void update(ProjectExplorer::Project *project,
                       QHash<QString, QString> uiHeaders);

private slots:
    void buildStateHasChanged(ProjectExplorer::Project *project);
    void projectWasRemoved(ProjectExplorer::Project *project);
    void editorIsAboutToClose(Core::IEditor *editor);
    void editorWasChanged(Core::IEditor *editor);
    void uiDocumentContentsHasChanged();

private:
    UiCodeModelManager();
    ~UiCodeModelManager();

    static void updateContents(const QString &uiFileName, const QString &contents);

    QHash<ProjectExplorer::Project *, QList<UiCodeModelSupport *> > m_projectUiSupport;
    Core::IEditor *m_lastEditor;
    bool m_dirty;

    static UiCodeModelManager *m_instance;

    friend class Internal::QtSupportPlugin;
};

} // QtSupport

#endif // UICODEMODELSUPPORT_H
