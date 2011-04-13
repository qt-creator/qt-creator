/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CPPTOOLS_H
#define CPPTOOLS_H

#include <extensionsystem/iplugin.h>
#include <projectexplorer/projectexplorer.h>
#include <find/ifindfilter.h>
#include <utils/filesearch.h>

#include <QtGui/QTextDocument>
#include <QtGui/QKeySequence>
#include <QtCore/QSharedPointer>
#include <QtCore/QFutureInterface>
#include <QtCore/QPointer>
#include <QtCore/QFutureWatcher>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QDir;
QT_END_NAMESPACE

namespace CppTools {
namespace Internal {

class CppModelManager;
struct CppFileSettings;

class CppToolsPlugin : public ExtensionSystem::IPlugin
{
    Q_DISABLE_COPY(CppToolsPlugin)
    Q_OBJECT
public:
    static CppToolsPlugin *instance() { return m_instance; }

    CppToolsPlugin();
    ~CppToolsPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
    CppModelManager *cppModelManager() { return m_modelManager; }
    QString correspondingHeaderOrSource(const QString &fileName) const;

private slots:
    void switchHeaderSource();

private:
    QString correspondingHeaderOrSourceI(const QString &fileName) const;
    QFileInfo findFile(const QDir &dir, const QString &name, const ProjectExplorer::Project *project) const;

    CppModelManager *m_modelManager;
    QSharedPointer<CppFileSettings> m_fileSettings;

    static CppToolsPlugin *m_instance;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPTOOLS_H
