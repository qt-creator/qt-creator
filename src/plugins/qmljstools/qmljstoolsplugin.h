/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSTOOLS_H
#define QMLJSTOOLS_H

#include <extensionsystem/iplugin.h>
#include <projectexplorer/projectexplorer.h>

#include <QtGui/QTextDocument>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QDir;
QT_END_NAMESPACE

namespace QmlJSTools {
namespace Internal {

class ModelManager;

class QmlJSToolsPlugin : public ExtensionSystem::IPlugin
{
    Q_DISABLE_COPY(QmlJSToolsPlugin)
    Q_OBJECT
public:
    static QmlJSToolsPlugin *instance() { return m_instance; }

    QmlJSToolsPlugin();
    ~QmlJSToolsPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
    ModelManager *modelManager() { return m_modelManager; }

private:
    ModelManager *m_modelManager;

    static QmlJSToolsPlugin *m_instance;
};

} // namespace Internal
} // namespace CppTools

#endif // QMLJSTOOLS_H
