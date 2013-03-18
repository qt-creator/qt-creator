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

#include "pythonsourcegenerator.h"
#include <QSet>

namespace PythonEditor {

static const char BASH_RUN_HEADER[] = "#!/usr/bin/env python\n";
static const char ENCODING_HEADER[] = "# -*- coding: utf-8 -*-\n";

SourceGenerator::SourceGenerator()
    : m_pythonQtBinding(PySide)
    , m_pythonQtVersion(Qt4)
{
}

SourceGenerator::~SourceGenerator()
{
}

void SourceGenerator::setPythonQtBinding(QtBinding binding)
{
    m_pythonQtBinding = binding;
}

void SourceGenerator::setPythonQtVersion(SourceGenerator::QtVersion version)
{
    m_pythonQtVersion = version;
}

QString SourceGenerator::generateClass(const QString &className,
                                       const QString &baseClass,
                                       Utils::NewClassWidget::ClassType classType) const
{
    QSet<QString> modules;
    bool hasUserBaseClass = !baseClass.isEmpty();
    // heuristic
    bool wasInheritedFromQt = hasUserBaseClass && (baseClass.at(0) == QLatin1Char('Q'));

    QString actualBase = baseClass;

    switch (classType) {
    case Utils::NewClassWidget::NoClassType:
        break;
    case Utils::NewClassWidget::SharedDataClass:
    case Utils::NewClassWidget::ClassInheritsQQuickItem:
        break;
    case Utils::NewClassWidget::ClassInheritsQObject:
        wasInheritedFromQt = true;
        modules.insert(QLatin1String("QtCore"));
        if (!hasUserBaseClass)
            actualBase = QLatin1String("QtCore.QObject");
        break;

    case Utils::NewClassWidget::ClassInheritsQWidget:
        wasInheritedFromQt = true;
        modules.insert(QLatin1String("QtCore"));
        modules.insert(moduleForQWidget());
        if (!hasUserBaseClass)
            actualBase = moduleForQWidget() + QLatin1String(".QWidget");
        break;

    case Utils::NewClassWidget::ClassInheritsQDeclarativeItem:
        wasInheritedFromQt = true;
        modules.insert(QLatin1String("QtCore"));
        modules.insert(QLatin1String("QtDeclarative"));
        if (!hasUserBaseClass)
            actualBase = QLatin1String("QtDeclarative.QDeclarativeItem");
        break;
    }

    QString nonQtModule; // empty
    if (hasUserBaseClass) {
        int dotIndex = baseClass.lastIndexOf(QLatin1Char('.'));
        if (dotIndex != -1) {
            if (wasInheritedFromQt)
                modules.insert(baseClass.left(dotIndex));
            else
                nonQtModule = baseClass.left(dotIndex);
        }
    }

    QString ret;
    ret.reserve(1024);
    ret += QLatin1String(ENCODING_HEADER);
    ret += QLatin1Char('\n');

    if (!modules.isEmpty()) {
        ret += qtModulesImport(modules);
        ret += QLatin1Char('\n');
    }

    if (!nonQtModule.isEmpty())
        ret += QString::fromLatin1("import %1\n\n").arg(nonQtModule);

    if (actualBase.isEmpty())
        ret += QString::fromLatin1("class %1:\n").arg(className);
    else
        ret += QString::fromLatin1("class %1(%2):\n").arg(className).arg(actualBase);

    ret += QLatin1String("    def __init__(self):\n");
    if (wasInheritedFromQt)
        ret += QString::fromLatin1("        %1.__init__(self)\n").arg(actualBase);
    ret += QLatin1String("        pass\n");

    return ret;
}

/**
* @brief Generates main file of PyQt/PySide application
*
* Class MainWindow should be defined in 'mainwindow' module.
* @param windowTitle Title for created window instance
*/
QString SourceGenerator::generateQtMain(const QString &windowTitle) const
{
    QSet<QString> qtModules;
    qtModules.insert(QLatin1String("QtCore"));
    qtModules.insert(moduleForQWidget());

    QString ret;
    ret.reserve(1024);
    ret += QLatin1String(BASH_RUN_HEADER);
    ret += QLatin1String(ENCODING_HEADER);
    ret += QLatin1Char('\n');
    ret += QLatin1String("import sys\n");
    ret += qtModulesImport(qtModules);
    ret += QLatin1String("from mainwindow import MainWindow\n");
    ret += QLatin1Char('\n');

    ret += QString::fromLatin1(
                "if __name__ == \'__main__\':\n"
                "    app = %1.QApplication(sys.argv)\n"
                "    win = MainWindow()\n"
                "    win.setWindowTitle(u\'%2\')\n"
                "    win.show()\n"
                "    app.exec_()\n"
                ).arg(moduleForQWidget()).arg(windowTitle);

    return ret;
}

QString SourceGenerator::qtModulesImport(const QSet<QString> &modules) const
{
    QString slotsImport;
    if (modules.contains(QLatin1String("QtCore")))
        slotsImport = QLatin1String("    from PyQt4.QtCore import pyqtSlot as Slot\n");

    QLatin1String defaultBinding("PySide");
    QLatin1String fallbackBinding("PyQt4");
    if (m_pythonQtBinding == PyQt)
        qSwap(defaultBinding, fallbackBinding);

    QString ret;
    ret.reserve(256);
    ret += QLatin1String("try:\n");
    if (m_pythonQtBinding == PyQt)
        ret += slotsImport;
    foreach (const QString &name, modules)
        ret += QString::fromLatin1("    from %1 import %2\n")
                .arg(defaultBinding)
                .arg(name);

    ret += QLatin1String("except:\n");
    if (m_pythonQtBinding != PyQt)
        ret += slotsImport;
    foreach (const QString &name, modules)
        ret += QString::fromLatin1("    from %1 import %2\n")
                .arg(fallbackBinding)
                .arg(name);

    return ret;
}

QString SourceGenerator::moduleForQWidget() const
{
    if (m_pythonQtVersion == Qt4)
        return QLatin1String("QtGui");
    else
        return QLatin1String("QtWidgets");
}

} // namespace PythonEditor
