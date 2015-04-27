/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MANAGER_H
#define MANAGER_H

#include "highlightdefinitionmetadata.h"

#include <QString>
#include <QHash>
#include <QSet>
#include <QUrl>
#include <QList>
#include <QPair>
#include <QSharedPointer>
#include <QFutureWatcher>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QStringList;
class QIODevice;
template <class> class QFutureInterface;
QT_END_NAMESPACE

namespace Utils { class MimeType; }

namespace TextEditor {
namespace Internal {

class HighlightDefinition;
class ManagerProcessor;
class MultiDefinitionDownloader;

// This is the generic highlighter manager. It is not thread-safe.

class Manager : public QObject
{
    Q_OBJECT
public:
    virtual ~Manager();
    static Manager *instance();

    QString definitionIdByName(const QString &name) const;
    QString definitionIdByMimeType(const Utils::MimeType &mimeType) const;
    QString definitionIdByFile(const QString &filePath) const;
    QString definitionIdByMimeTypeAndFile(const Utils::MimeType &mimeType,
                                          const QString &filePath) const;
    DefinitionMetaDataPtr availableDefinitionByName(const QString &name) const;

    bool isBuildingDefinition(const QString &id) const;
    QSharedPointer<HighlightDefinition> definition(const QString &id);
    DefinitionMetaDataPtr definitionMetaData(const QString &id) const;

    void downloadAvailableDefinitionsMetaData();
    void downloadDefinitions(const QList<QUrl> &urls, const QString &savePath);
    bool isDownloadingDefinitions() const;

    static DefinitionMetaDataPtr parseMetadata(const QFileInfo &fileInfo);

public slots:
    void registerHighlightingFiles();

private slots:
    void registerHighlightingFilesFinished();
    void downloadAvailableDefinitionsListFinished();
    void downloadDefinitionsFinished();

signals:
    void highlightingFilesRegistered();

private:
    Manager();

    void clear();

    MultiDefinitionDownloader *m_multiDownloader;
    QList<DefinitionMetaDataPtr> parseAvailableDefinitionsList(QIODevice *device);

    QSet<QString> m_isBuildingDefinition;
    QHash<QString, QSharedPointer<HighlightDefinition> > m_definitions;
    QHash<QString, DefinitionMetaDataPtr> m_availableDefinitions;

    struct RegisterData
    {
        QHash<QString, QString> m_idByName;
        QHash<QString, QString> m_idByMimeType;
        QHash<QString, DefinitionMetaDataPtr> m_definitionsMetaData;
    };
    RegisterData m_register;
    bool m_hasQueuedRegistration;
    QFutureWatcher<RegisterData> m_registeringWatcher;
    friend class ManagerProcessor;

signals:
    void definitionsMetaDataReady(const QList<Internal::DefinitionMetaDataPtr>&);
    void errorDownloadingDefinitionsMetaData();
};

} // namespace Internal
} // namespace TextEditor

#endif // MANAGER_H
