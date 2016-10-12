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

#pragma once

#include <QObject>
#include <QList>
#include <QLoggingCategory>

#include <designdocument.h>

namespace Core { class IEditor; }
namespace ProjectExplorer { class Node; }
namespace ProjectExplorer { class Project; }
namespace QmlDesigner {

Q_DECLARE_LOGGING_CATEGORY(documentManagerLog)

class QMLDESIGNERCORE_EXPORT DocumentManager : public QObject
{
    Q_OBJECT
public:
    DocumentManager();
    ~DocumentManager();

    void setCurrentDesignDocument(Core::IEditor*editor);
    DesignDocument *currentDesignDocument() const;
    bool hasCurrentDesignDocument() const;

    void removeEditors(const QList<Core::IEditor *> &editors);

    static void goIntoComponent(const ModelNode &modelNode);

    static bool createFile(const QString &filePath, const QString &contents);
    static void addFileToVersionControl(const QString &directoryPath, const QString &newFilePath);
    static Utils::FileName currentFilePath();

    static QStringList isoIconsQmakeVariableValue(const QString &proPath);
    static bool setIsoIconsQmakeVariableValue(const QString &proPath, const QStringList &value);
    static void findPathToIsoProFile(bool *iconResourceFileAlreadyExists, QString *resourceFilePath,
        QString *resourceFileProPath, const QString &isoIconsQrcFile);
    static bool isoProFileSupportsAddingExistingFiles(const QString &resourceFileProPath);
    static bool addResourceFileToIsoProject(const QString &resourceFileProPath, const QString &resourceFilePath);

private:
    QHash<Core::IEditor *,QPointer<DesignDocument> > m_designDocumentHash;
    QPointer<DesignDocument> m_currentDesignDocument;
};

} // namespace QmlDesigner
