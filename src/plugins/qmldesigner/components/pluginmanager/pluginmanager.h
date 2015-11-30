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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "pluginpath.h"

#include <QObject>
#include <QList>

QT_BEGIN_NAMESPACE
class QString;
class QAbstractItemModel;
class QDialog;
QT_END_NAMESPACE

namespace QmlDesigner {

class IPlugin;

// PluginManager: Loads the plugin libraries on demand "as lazy as
// possible", that is, directories are scanned and
// instances are created only when  instances() is called.

class PluginManager
{
    Q_DISABLE_COPY(PluginManager)

    typedef QList<PluginPath> PluginPathList;

public:
    typedef QList<IPlugin *> IPluginList;

    PluginManager();

    void setPluginPaths(const QStringList &paths);

    IPluginList instances();

    QDialog *createAboutPluginDialog(QWidget *parent);

private: // functions
    // Convenience to create a model for an "About Plugins"
    // dialog. Forces plugin initialization.
    QAbstractItemModel *createModel(QObject *parent = 0);

private: // variables
    PluginPathList m_paths;
};

} // namespace QmlDesigner
#endif // PLUGINMANAGER_H
