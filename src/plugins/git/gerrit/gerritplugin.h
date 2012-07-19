/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef GERRIT_INTERNAL_GERRITPLUGIN_H
#define GERRIT_INTERNAL_GERRITPLUGIN_H

#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>

namespace Core {
class ActionContainer;
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

    static QString gitBinary();
    static QString branch(const QString &repository);

public slots:
    void fetchDisplay(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchApply(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchCheckout(const QSharedPointer<Gerrit::Internal::GerritChange> &change);

private slots:
    void openView();

private:
    QString findLocalRepository(QString project, const QString &branch) const;
    void fetch(const QSharedPointer<Gerrit::Internal::GerritChange> &change, int mode);

    QSharedPointer<GerritParameters> m_parameters;
    QWeakPointer<GerritDialog> m_dialog;
};

} // namespace Internal
} // namespace Gerrit

#endif // GERRIT_INTERNAL_GERRITPLUGIN_H
