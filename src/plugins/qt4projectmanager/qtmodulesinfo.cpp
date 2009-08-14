/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qtmodulesinfo.h"
#include "qglobal.h"
#include <QtCore/QtDebug>
#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QCoreApplication>

using namespace Qt4ProjectManager::Internal;

struct item
{
    const char * const config;
    const QString name;
    const QString description;
    bool isDefault;
};

typedef QVector<const item*> itemVectorType;
typedef QHash<QString, const item*> itemHashType;

const itemVectorType itemVector()
{
    static const struct item items[] = {
        {"core",
            QCoreApplication::translate("QtModulesInfo", "QtCore Module"),
            QCoreApplication::translate("QtModulesInfo", "Core non-GUI classes used by other modules"),
            true},
        {"gui",
            QCoreApplication::translate("QtModulesInfo", "QtGui Module"),
            QCoreApplication::translate("QtModulesInfo", "Graphical user interface components"),
            true},
        {"network",
            QCoreApplication::translate("QtModulesInfo", "QtNetwork Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes for network programming"),
            false},
        {"opengl",
            QCoreApplication::translate("QtModulesInfo", "QtOpenGL Module"),
            QCoreApplication::translate("QtModulesInfo", "OpenGL support classes"),
            false},
        {"sql",
            QCoreApplication::translate("QtModulesInfo", "QtSql Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes for database integration using SQL"),
            false},
        {"script",
            QCoreApplication::translate("QtModulesInfo", "QtScript Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes for evaluating Qt Scripts"),
            false},
        {"svg",
            QCoreApplication::translate("QtModulesInfo", "QtSvg Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes for displaying the contents of SVG files"),
            false},
        {"webkit",
            QCoreApplication::translate("QtModulesInfo", "QtWebKit Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes for displaying and editing Web content"),
            false},
        {"xml",
            QCoreApplication::translate("QtModulesInfo", "QtXml Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes for handling XML"),
            false},
        {"xmlpatterns",
            QCoreApplication::translate("QtModulesInfo", "QtXmlPatterns Module"),
            QCoreApplication::translate("QtModulesInfo", "An XQuery/XPath engine for XML and custom data models"),
            false},
        {"phonon",
            QCoreApplication::translate("QtModulesInfo", "Phonon Module"),
            QCoreApplication::translate("QtModulesInfo", "Multimedia framework classes"),
            false},
        {"qt3support",
            QCoreApplication::translate("QtModulesInfo", "Qt3Support Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes that ease porting from Qt 3 to Qt 4"),
            false},
        {"testlib",
            QCoreApplication::translate("QtModulesInfo", "QtTest Module"),
            QCoreApplication::translate("QtModulesInfo", "Tool classes for unit testing"),
            false},
        {"dbus",
            QCoreApplication::translate("QtModulesInfo", "QtDBus Module"),
            QCoreApplication::translate("QtModulesInfo", "Classes for Inter-Process Communication using the D-Bus"),
            false}
    };
    const int itemsCount = sizeof items / sizeof items[0];
    itemVectorType result;
    result.reserve(itemsCount);
    for (int i = 0; i < itemsCount; i++)
        result.append(items + i);
    return result;
}

Q_GLOBAL_STATIC_WITH_INITIALIZER(itemVectorType, staticItemVector, {
    *x = itemVector();
});

Q_GLOBAL_STATIC_WITH_INITIALIZER(QStringList, staticModulesList, {
    const itemVectorType * const itemVector = staticItemVector();
    for (int i = 0; i < itemVector->count(); i++)
        x->append(QString::fromLatin1(itemVector->at(i)->config));
});

Q_GLOBAL_STATIC_WITH_INITIALIZER(itemHashType, staticItemHash, {
    const itemVectorType * const itemVector = staticItemVector();
    for (int i = 0; i < itemVector->count(); i++)
        x->insert(QString::fromLatin1(itemVector->at(i)->config), itemVector->at(i));
});

QStringList QtModulesInfo::modules()
{
    return *staticModulesList();
}

static inline const item *itemForModule(const QString &module)
{
    return staticItemHash()->value(module.toLatin1().data());
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
