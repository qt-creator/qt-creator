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
    void registerHighlightingFiles();

    static DefinitionMetaDataPtr parseMetadata(const QFileInfo &fileInfo);

    struct RegisterData
    {
        QHash<QString, QString> m_idByName;
        QHash<QString, QString> m_idByMimeType;
        QHash<QString, DefinitionMetaDataPtr> m_definitionsMetaData;
    };
private:
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
