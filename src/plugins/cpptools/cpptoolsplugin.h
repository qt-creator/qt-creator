/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPPTOOLS_H
#define CPPTOOLS_H

#include <extensionsystem/iplugin.h>
#include <projectexplorer/ProjectExplorerInterfaces>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QDir;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace CppTools {
namespace Internal {

class CppCodeCompletion;
class CppModelManager;

class CppToolsPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    static CppToolsPlugin *instance() { return m_instance; }

    CppToolsPlugin();
    ~CppToolsPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    void shutdown();
    CppModelManager *cppModelManager() { return m_modelManager; }
    QString correspondingHeaderOrSource(const QString &fileName) const;

private slots:
    void switchHeaderSource();

private:
    QString correspondingHeaderOrSourceI(const QString &fileName) const;
    QFileInfo findFile(const QDir &dir, const QString &name, const ProjectExplorer::Project *project) const;

    Core::ICore *m_core;
    int m_context;
    CppModelManager *m_modelManager;
    CppCodeCompletion *m_completion;

    static CppToolsPlugin *m_instance;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPTOOLS_H
