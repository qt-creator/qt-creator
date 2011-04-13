/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "valgrindsettings.h"

#include "valgrindconfigwidget.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QtCore/QSettings>

using namespace Analyzer::Internal;
using namespace Analyzer;

static const QLatin1String groupC("Analyzer");

static const QLatin1String valgrindExeC("Analyzer.Valgrind.ValgrindExecutable");

ValgrindSettings::ValgrindSettings(QObject *parent)
: AbstractAnalyzerSubConfig(parent)
{
}

ValgrindSettings::~ValgrindSettings()
{}

QVariantMap ValgrindSettings::defaults() const
{
    QVariantMap map;
    map.insert(valgrindExeC, QLatin1String("valgrind"));
    return map;
}

bool ValgrindSettings::fromMap(const QVariantMap &map)
{
    setIfPresent(map, valgrindExeC, &m_valgrindExecutable);
    return true;
}

QVariantMap ValgrindSettings::toMap() const
{
    QVariantMap map;
    map.insert(valgrindExeC, m_valgrindExecutable);

    return map;
}

void ValgrindSettings::setValgrindExecutable(const QString &valgrindExecutable)
{
    if (m_valgrindExecutable != valgrindExecutable) {
        m_valgrindExecutable = valgrindExecutable;
        emit valgrindExecutableChanged(valgrindExecutable);
    }
}

QString ValgrindSettings::valgrindExecutable() const
{
    return m_valgrindExecutable;
}

QString ValgrindSettings::id() const
{
    return "Analyzer.Valgrind.Settings.Generic";
}

QString ValgrindSettings::displayName() const
{
    return tr("Generic Settings");
}

QWidget *ValgrindSettings::createConfigWidget(QWidget *parent)
{
    return new ValgrindConfigWidget(this, parent);
}
