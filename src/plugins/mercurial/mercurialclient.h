/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIALCLIENT_H
#define MERCURIALCLIENT_H

#include <QtCore/QObject>
#include <QtCore/QPair>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace VCSBase{
class VCSBaseEditor;
}

namespace Mercurial {
namespace Internal {

class MercurialJobRunner;

class MercurialClient : public QObject
{
    Q_OBJECT
public:
    MercurialClient();
    ~MercurialClient();
    bool add(const QString &fileName);
    bool remove(const QString &fileName);
    bool manifestSync(const QString &filename);
    QString branchQuerySync(const QFileInfo &repositoryRoot);
    void annotate(const QFileInfo &file);
    void diff(const QFileInfo &fileOrDir);
    void log(const QFileInfo &fileOrDir);
    void import(const QFileInfo &repositoryRoot, const QStringList &files);
    void pull(const QFileInfo &repositoryRoot, const QString &repository);
    void push(const QFileInfo &repositoryRoot, const QString &repository);
    void incoming(const QFileInfo &repositoryRoot, const QString &repository);
    void outgoing(const QFileInfo &repositoryRoot);
    void status(const QFileInfo &fileOrDir);
    void statusWithSignal(const QFileInfo &fileOrDir);
    void revert(const QFileInfo &fileOrDir, const QString &revision);
    void update(const QFileInfo &repositoryRoot, const QString &revision);
    void commit(const QFileInfo &repositoryRoot, const QStringList &files,
                const QString &commiterInfo, const QString &commitMessageFile);

    static QString findTopLevelForFile(const QFileInfo &file);

signals:
    void parsedStatus(const QList<QPair<QString, QString> > &statusList);

public slots:
    void view(const QString &source, const QString &id);
    void settingsChanged();

private slots:
    void statusParser(const QByteArray &data);

private:
    bool executeHgSynchronously(const QFileInfo &file, const QStringList &args,
                                QByteArray *output=0) const;

    MercurialJobRunner *jobManager;
    Core::ICore *core;

    VCSBase::VCSBaseEditor *createVCSEditor(const QString &kind, QString title,
                                            const QString &source, bool setSourceCodec,
                                            const char *registerDynamicProperty,
                                            const QString &dynamicPropertyValue) const;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALCLIENT_H
