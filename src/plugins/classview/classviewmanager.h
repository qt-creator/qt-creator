/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov
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

#ifndef CLASSVIEWMANAGER_H
#define CLASSVIEWMANAGER_H

#include <QObject>
#include <QSharedPointer>
#include <QStandardItem>

#include <coreplugin/id.h>
#include <cplusplus/CppDocument.h>

namespace ClassView {
namespace Internal {

class ManagerPrivate;

class Manager : public QObject
{
    Q_OBJECT

public:
    explicit Manager(QObject *parent = 0);

    virtual ~Manager();

    //! Get an instance of Manager
    static Manager *instance();

    bool canFetchMore(QStandardItem *item, bool skipRoot = false) const;

    void fetchMore(QStandardItem *item, bool skipRoot = false);

    bool hasChildren(QStandardItem *item) const;

signals:
    void stateChanged(bool state);

    void treeDataUpdate(QSharedPointer<QStandardItem> result);

    void requestTreeDataUpdate();

    void requestDocumentUpdated(CPlusPlus::Document::Ptr doc);

    void requestResetCurrentState();

    void requestClearCache();

    void requestClearCacheAll();

    void requestSetFlatMode(bool flat);

public slots:
    void gotoLocation(const QString &fileName, int line = 0, int column = 0);

    void gotoLocations(const QList<QVariant> &locations);

    void onRequestTreeDataUpdate();

    void setFlatMode(bool flat);

protected slots:
    void onWidgetVisibilityIsChanged(bool visibility);
    void onStateChanged(bool state);
    void onProjectListChanged();
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void onTreeDataUpdate(QSharedPointer<QStandardItem> result);

protected:
    //! Perform an initialization
    void initialize();

    inline bool state() const;

    void setState(bool state);

private:
    //! private class data pointer
    ManagerPrivate *d;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWMANAGER_H
