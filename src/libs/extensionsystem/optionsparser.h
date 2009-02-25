/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef OPTIONSPARSER_H
#define OPTIONSPARSER_H

#include "pluginmanager_p.h"

#include <QtCore/QStringList>
#include <QtCore/QMap>

namespace ExtensionSystem {
namespace Internal {

class OptionsParser
{
public:
    OptionsParser(const QStringList &args,
        const QMap<QString, bool> &appOptions,
        QMap<QString, QString> *foundAppOptions,
        QString *errorString,
        PluginManagerPrivate *pmPrivate);
    
    bool parse();

    static const char *NO_LOAD_OPTION;
    static const char *TEST_OPTION;
private:
    // return value indicates if the option was processed
    // it doesn't indicate success (--> m_hasError)
    bool checkForEndOfOptions();
    bool checkForNoLoadOption();
    bool checkForTestOption();
    bool checkForAppOption();
    bool checkForPluginOption();
    bool checkForUnknownOption();

    enum TokenType { OptionalToken, RequiredToken };
    bool nextToken(TokenType type = OptionalToken);
    
    const QStringList &m_args;
    const QMap<QString, bool> &m_appOptions;
    QMap<QString, QString> *m_foundAppOptions;
    QString *m_errorString;
    PluginManagerPrivate *m_pmPrivate;
    
    // state
    QString m_currentArg;
    QStringList::const_iterator m_it;
    QStringList::const_iterator m_end;
    bool m_isDependencyRefreshNeeded;
    bool m_hasError;
};

} // namespace Internal
} // namespace ExtensionSystem

#endif // OPTIONSPARSER_H
