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

#ifndef DEFINITIONDOWNLOADER_H
#define DEFINITIONDOWNLOADER_H

#include <QObject>
#include <QString>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QNetworkReply;
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

signals:
    void foundReferencedDefinition(const QString &name);

private:
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
