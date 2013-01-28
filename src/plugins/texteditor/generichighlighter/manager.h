/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef MANAGER_H
#define MANAGER_H

#include "highlightdefinitionmetadata.h"

#include <coreplugin/mimedatabase.h>

#include <QString>
#include <QHash>
#include <QSet>
#include <QUrl>
#include <QList>
#include <QPair>
#include <QSharedPointer>
#include <QFutureWatcher>

#include <utils/networkaccessmanager.h>

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
class ManagerProcessor;

// This is the generic highlighter manager. It is not thread-safe.

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
    void downloadDefinitions(const QList<QUrl> &urls, const QString &savePath);
    bool isDownloadingDefinitions() const;

    static QSharedPointer<HighlightDefinitionMetaData> parseMetadata(const QFileInfo &fileInfo);

public slots:
    void registerMimeTypes();

private slots:
    void registerMimeTypesFinished();
    void downloadAvailableDefinitionsListFinished();
    void downloadDefinitionsFinished();

signals:
    void mimeTypesRegistered();

private:
    Manager();

    void clear();

    bool m_isDownloadingDefinitionsSpec;
    QList<DefinitionDownloader *> m_downloaders;
    QFutureWatcher<void> m_downloadWatcher;
    Utils::NetworkAccessManager m_networkManager;
    QList<HighlightDefinitionMetaData> parseAvailableDefinitionsList(QIODevice *device) const;

    QSet<QString> m_isBuildingDefinition;
    QHash<QString, QSharedPointer<HighlightDefinition> > m_definitions;

    struct RegisterData
    {
        QHash<QString, QString> m_idByName;
        QHash<QString, QString> m_idByMimeType;
        QHash<QString, QSharedPointer<HighlightDefinitionMetaData> > m_definitionsMetaData;
    };
    RegisterData m_register;
    bool m_hasQueuedRegistration;
    QFutureWatcher<QPair<RegisterData, QList<Core::MimeType> > > m_registeringWatcher;
    friend class ManagerProcessor;

signals:
    void definitionsMetaDataReady(const QList<Internal::HighlightDefinitionMetaData>&);
    void errorDownloadingDefinitionsMetaData();
};

} // namespace Internal
} // namespace TextEditor

#endif // MANAGER_H
