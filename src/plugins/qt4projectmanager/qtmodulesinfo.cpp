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

#include "qtmodulesinfo.h"
#include "qglobal.h"
#include <QDebug>
#include <QString>
#include <QHash>
#include <QCoreApplication>

using namespace Qt4ProjectManager::Internal;

struct item
{
    const char * const config;
    const QString name;
    const QString description;
    bool isDefault;
};

static inline QVector<const item*> itemVector()
{
    static const struct item items[] = {
        {"core",
            QLatin1String("QtCore"),
            QCoreApplication::translate("QtModulesInfo", "Core non-GUI classes used by other modules"),
            true},
        {"gui",
            QLatin1String("QtGui"),
            QCoreApplication::translate("QtModulesInfo", "Graphical user interface components"),
            true},
        {"network",
            QLatin1String("QtNetwork"),
            QCoreApplication::translate("QtModulesInfo", "Classes for network programming"),
            false},
        {"opengl",
            QLatin1String("QtOpenGL"),
            QCoreApplication::translate("QtModulesInfo", "OpenGL support classes"),
            false},
        {"sql",
            QLatin1String("QtSql"),
            QCoreApplication::translate("QtModulesInfo", "Classes for database integration using SQL"),
            false},
        {"script",
            QLatin1String("QtScript"),
            QCoreApplication::translate("QtModulesInfo", "Classes for evaluating Qt Scripts"),
            false},
        {"scripttools",
            QLatin1String("QtScriptTools"),
            QCoreApplication::translate("QtModulesInfo", "Additional Qt Script components"),
            false},
        {"svg",
            QLatin1String("QtSvg"),
            QCoreApplication::translate("QtModulesInfo", "Classes for displaying the contents of SVG files"),
            false},
        {"webkit",
            QLatin1String("QtWebKit"),
            QCoreApplication::translate("QtModulesInfo", "Classes for displaying and editing Web content"),
            false},
        {"xml",
            QLatin1String("QtXml"),
            QCoreApplication::translate("QtModulesInfo", "Classes for handling XML"),
            false},
        {"xmlpatterns",
            QLatin1String("QtXmlPatterns"),
            QCoreApplication::translate("QtModulesInfo", "An XQuery/XPath engine for XML and custom data models"),
            false},
        {"phonon",
            QLatin1String("Phonon"),
            QCoreApplication::translate("QtModulesInfo", "Multimedia framework classes"),
            false},
        {"multimedia",
            QLatin1String("QtMultimedia"),
            QCoreApplication::translate("QtModulesInfo", "Classes for low-level multimedia functionality"),
            false},
        {"qt3support",
            QLatin1String("Qt3Support"),
            QCoreApplication::translate("QtModulesInfo", "Classes that ease porting from Qt 3 to Qt 4"),
            false},
        {"testlib",
            QLatin1String("QtTest"),
            QCoreApplication::translate("QtModulesInfo", "Tool classes for unit testing"),
            false},
        {"dbus",
            QLatin1String("QtDBus"),
            QCoreApplication::translate("QtModulesInfo", "Classes for Inter-Process Communication using the D-Bus"),
            false}
    };
    const int itemsCount = sizeof items / sizeof items[0];
    QVector<const item*> result;
    result.reserve(itemsCount);
    for (int i = 0; i < itemsCount; i++)
        result.append(items + i);
    return result;
}

class StaticQtModuleInfo
{
public:
    StaticQtModuleInfo() : items(itemVector()) {}

    const QVector<const item*> items;
};

Q_GLOBAL_STATIC(StaticQtModuleInfo, staticQtModuleInfo)

QStringList QtModulesInfo::modules()
{
    QStringList result;
    foreach (const item *i, staticQtModuleInfo()->items)
        result.push_back(QLatin1String(i->config));
    return result;
}

static inline const item *itemForModule(const QString &module)
{
    foreach (const item *i, staticQtModuleInfo()->items)
        if (QLatin1String(i->config) == module)
            return i;
    return 0;
}

QString QtModulesInfo::moduleName(const QString &module)
{
    const item * const i = itemForModule(module);
    return i?i->name:QString();
}

QString QtModulesInfo::moduleDescription(const QString &module)
{
    const item * const i = itemForModule(module);
    return i?i->description:QString();
}

bool QtModulesInfo::moduleIsDefault(const QString &module)
{
    const item * const i = itemForModule(module);
    return i?i->isDefault:false;
}
