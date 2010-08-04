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

#ifndef DEFINITIONDOWNLOADER_H
#define DEFINITIONDOWNLOADER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QNetworkAccessManager;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {

class DefinitionDownloader : public QObject
{
    Q_OBJECT
public:
    DefinitionDownloader(const QUrl &url, const QString &localPath);

    enum Status {
        NetworkError,
        WriteError,
        Ok,
        Unknown
    };

    void run();
    Status status() const;

private:
    QNetworkReply *getData(QNetworkAccessManager *manager) const;
    void saveData(QNetworkReply *reply);

    QUrl m_url;
    QString m_localPath;
    Status m_status;
};

// Currently QtConcurrent::map does not support passing member functions for sequence of pointers
// (only works for operator.*) which is the case for the downloaders held by the manager. Then the
// reason for the following functor. If something is implemented (for example a type traits) to
// handle operator->* in QtConcurrent::map this functor will not be necessary since it would be
// possible to directly pass DefinitionDownloader::run.
struct DownloaderStarter
{
    void operator()(DefinitionDownloader *downloader)
    { downloader->run(); }
};

} // namespace Internal
} // namespace TextEditor

#endif // DEFINITIONDOWNLOADER_H
