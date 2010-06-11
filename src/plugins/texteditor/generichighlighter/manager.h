/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef MANAGER_H
#define MANAGER_H

#include "highlightdefinitionmetadata.h"

#include <coreplugin/mimedatabase.h>

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QFutureWatcher>
#include <QtNetwork/QNetworkAccessManager>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QStringList;
class QIODevice;
template <class> class QFutureInterface;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {

class HighlightDefinition;
class DefinitionDownloader;

class Manager : public QObject
{
    Q_OBJECT
public:
    virtual ~Manager();
    static Manager *instance();

    QString definitionIdByName(const QString &name) const;
    QString definitionIdByMimeType(const QString &mimeType) const;
    QString definitionIdByAnyMimeType(const QStringList &mimeTypes) const;

    bool isBuildingDefinition(const QString &id) const;

    QSharedPointer<HighlightDefinition> definition(const QString &id);
    QSharedPointer<HighlightDefinitionMetaData> definitionMetaData(const QString &id) const;

    void downloadAvailableDefinitionsMetaData();
    void downloadDefinitions(const QList<QUrl> &urls);
    bool isDownloadingDefinitions() const;

public slots:
    void registerMimeTypes();
    void showGenericHighlighterOptions() const;

private slots:
    void registerMimeType(int index) const;
    void downloadAvailableDefinitionsListFinished();
    void downloadDefinitionsFinished();

private:
    Manager();
    Q_DISABLE_COPY(Manager)

    void gatherDefinitionsMimeTypes(QFutureInterface<Core::MimeType> &future);
    QSharedPointer<HighlightDefinitionMetaData> parseMetadata(const QFileInfo &fileInfo);
    QList<HighlightDefinitionMetaData> parseAvailableDefinitionsList(QIODevice *device) const;
    void clear();

    QHash<QString, QString> m_idByName;
    QHash<QString, QString> m_idByMimeType;
    QHash<QString, QSharedPointer<HighlightDefinition> > m_definitions;
    QHash<QString, QSharedPointer<HighlightDefinitionMetaData> > m_definitionsMetaData;
    QSet<QString> m_isBuilding;

    QList<DefinitionDownloader *> m_downloaders;
    bool m_downloadingDefinitions;
    QNetworkAccessManager m_networkManager;

    QFutureWatcher<void> m_downloadWatcher;
    QFutureWatcher<Core::MimeType> m_mimeTypeWatcher;

signals:
    void definitionsMetaDataReady(const QList<Internal::HighlightDefinitionMetaData>&);
    void errorDownloadingDefinitionsMetaData();
};

} // namespace Internal
} // namespace TextEditor

#endif // MANAGER_H
