/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SUBVERSIONCONTROL_H
#define SUBVERSIONCONTROL_H

#include <coreplugin/iversioncontrol.h>

namespace Subversion {
namespace Internal {

class SubversionPlugin;

// Just a proxy for SubversionPlugin
class SubversionControl : public Core::IVersionControl
{
    Q_OBJECT
public:
    explicit SubversionControl(SubversionPlugin *plugin);
    virtual QString displayName() const;

    virtual bool managesDirectory(const QString &directory, QString *topLevel = 0) const;

    virtual bool supportsOperation(Operation operation) const;
    virtual bool vcsOpen(const QString &fileName);
    virtual bool vcsAdd(const QString &fileName);
    virtual bool vcsDelete(const QString &filename);
    virtual bool vcsMove(const QString &from, const QString &to);
    virtual bool vcsCreateRepository(const QString &directory);
    virtual bool vcsCheckout(const QString &directory, const QByteArray &url);
    virtual QString vcsGetRepositoryURL(const QString &directory);

    virtual QString vcsCreateSnapshot(const QString &topLevel);
    virtual QStringList vcsSnapshots(const QString &topLevel);
    virtual bool vcsRestoreSnapshot(const QString &topLevel, const QString &name);
    virtual bool vcsRemoveSnapshot(const QString &topLevel, const QString &name);

    virtual bool vcsAnnotate(const QString &file, int line);

    void emitRepositoryChanged(const QString &);
    void emitFilesChanged(const QStringList &);

private:
    bool m_enabled;
    SubversionPlugin *m_plugin;
};

} // namespace Internal
} // namespace Subversion

#endif // SUBVERSIONCONTROL_H
