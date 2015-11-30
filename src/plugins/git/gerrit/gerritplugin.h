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

#ifndef GERRIT_INTERNAL_GERRITPLUGIN_H
#define GERRIT_INTERNAL_GERRITPLUGIN_H

#include <utils/fileutils.h>

#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QPair>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class ActionContainer;
class Command;
class CommandLocator;
}

namespace Gerrit {
namespace Internal {

class GerritChange;
class GerritParameters;
class GerritDialog;

class GerritPlugin : public QObject
{
    Q_OBJECT
public:
    explicit GerritPlugin(QObject *parent = 0);
    ~GerritPlugin();

    bool initialize(Core::ActionContainer *ac);

    static Utils::FileName gitBinDirectory();
    static QString branch(const QString &repository);
    void addToLocator(Core::CommandLocator *locator);
    void push(const QString &topLevel);

    void fetchDisplay(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchCherryPick(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchCheckout(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void updateActions(bool hasTopLevel);

signals:
    void fetchStarted(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchFinished();

private:
    void openView();
    void push();

    QString findLocalRepository(QString project, const QString &branch) const;
    void fetch(const QSharedPointer<GerritChange> &change, int mode);

    QSharedPointer<GerritParameters> m_parameters;
    QPointer<GerritDialog> m_dialog;
    Core::Command *m_gerritCommand;
    Core::Command *m_pushToGerritCommand;
    QString m_reviewers;
};

} // namespace Internal
} // namespace Gerrit

#endif // GERRIT_INTERNAL_GERRITPLUGIN_H
