/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qmlpreviewplugin.h"
#include "qmlpreviewclient.h"
#include "qmldebugtranslationclient.h"
#include "qmlpreviewfileontargetfinder.h"

#include <qmldebug/qmldebugconnectionmanager.h>
#include <utils/fileinprojectfinder.h>
#include <utils/filesystemwatcher.h>

namespace ProjectExplorer {
class Target;
}

namespace QmlPreview {
namespace Internal {

class QmlPreviewConnectionManager : public QmlDebug::QmlDebugConnectionManager
{
    Q_OBJECT
public:
    virtual ~QmlPreviewConnectionManager();

    explicit QmlPreviewConnectionManager(QObject *parent = nullptr);
    void setTarget(ProjectExplorer::Target *target);
    void setFileLoader(QmlPreviewFileLoader fileLoader);
    void setFileClassifier(QmlPreviewFileClassifier fileClassifier);
    void setFpsHandler(QmlPreviewFpsHandler fpsHandler);

signals:
    void loadFile(const QString &filename, const QString &changedFile, const QByteArray &contents);
    void zoom(float zoomFactor);
    void language(const QString &locale);
    void rerun();
    void restart();

protected:
    void createClients() override;
    void destroyClients() override;

private:
    void createPreviewClient();
    void createDebugTranslationClient();
    QUrl findValidI18nDirectoryAsUrl(const QString &locale);
    void clearClient(QObject *client);
    Utils::FileInProjectFinder m_projectFileFinder;
    QmlPreviewFileOnTargetFinder m_targetFileFinder;
    QPointer<QmlPreviewClient> m_qmlPreviewClient;
    QPointer<QmlDebugTranslationClient> m_qmlDebugTranslationClient;
    Utils::FileSystemWatcher m_fileSystemWatcher;
    QUrl m_lastLoadedUrl;
    QmlPreviewFileLoader m_fileLoader = nullptr;
    QmlPreviewFileClassifier m_fileClassifier = nullptr;
    QmlPreviewFpsHandler m_fpsHandler = nullptr;
};

} // namespace Internal
} // namespace QmlPreview
