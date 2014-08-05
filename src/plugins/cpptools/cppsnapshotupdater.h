/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLS_INTERNAL_SNAPSHOTUPDATER_H
#define CPPTOOLS_INTERNAL_SNAPSHOTUPDATER_H

#include "cpptools_global.h"
#include "cppmodelmanager.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/DependencyTable.h>

#include <QMutex>
#include <QString>

namespace CppTools {

class CPPTOOLS_EXPORT SnapshotUpdater
{
    Q_DISABLE_COPY(SnapshotUpdater)

public:
    SnapshotUpdater(const QString &fileInEditor = QString());

    QString fileInEditor() const
    { return m_fileInEditor; }

    void update(CppModelManagerInterface::WorkingCopy workingCopy);
    void releaseSnapshot();

    CPlusPlus::Document::Ptr document() const;
    CPlusPlus::Snapshot snapshot() const;
    ProjectPart::HeaderPaths headerPaths() const;

    ProjectPart::Ptr currentProjectPart() const;
    void setProjectPart(ProjectPart::Ptr projectPart);

    void setUsePrecompiledHeaders(bool usePrecompiledHeaders);
    void setEditorDefines(const QByteArray &editorDefines);

    void setReleaseSourceAndAST(bool onoff);

private:
    void updateProjectPart();
    void addFileAndDependencies(QSet<QString> *toRemove, const QString &fileName) const;

private:
    mutable QMutex m_mutex;
    const QString m_fileInEditor;
    ProjectPart::Ptr m_projectPart, m_manuallySetProjectPart;
    QByteArray m_configFile;
    bool m_editorDefinesChangedSinceLastUpdate;
    QByteArray m_editorDefines;
    ProjectPart::HeaderPaths m_headerPaths;
    QString m_projectConfigFile;
    QStringList m_precompiledHeaders;
    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::DependencyTable m_deps;
    bool m_usePrecompiledHeaders;
    bool m_forceSnapshotInvalidation;
    bool m_releaseSourceAndAST;
};

} // namespace CppTools

#endif // CPPTOOLS_INTERNAL_SNAPSHOTUPDATER_H
