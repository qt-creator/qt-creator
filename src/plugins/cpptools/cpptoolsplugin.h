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

namespace CPlusPlus {
class Snapshot;
}

namespace Find {
class SearchResultWindow;
struct SearchResultItem;
}

namespace CppTools {
namespace Internal {

class CppCodeCompletion;
class CppModelManager;
struct CppFileSettings;

class FindClassDeclarations: public Find::IFindFilter
{
    Q_OBJECT

public:
    FindClassDeclarations(CppModelManager *modelManager);

    // Find::IFindFilter
    virtual QString id() const { return QLatin1String("CppTools.Find.ClassDeclarations"); }
    virtual QString name() const { return tr("Class Declarations"); }
    virtual bool isEnabled() const { return true; }
    virtual QKeySequence defaultShortcut() const { return QKeySequence(); }
    virtual void findAll(const QString &txt, QTextDocument::FindFlags findFlags);

protected Q_SLOTS:
    void displayResult(int);
    void searchFinished();
    void openEditor(const Find::SearchResultItem &item);

private:
    QPointer<CppModelManager> _modelManager;
    Find::SearchResultWindow *_resultWindow;
    QFutureWatcher<Core::Utils::FileSearchResult> m_watcher;
};

class FindFunctionCalls: public Find::IFindFilter // ### share code with FindClassDeclarations
{
    Q_OBJECT

public:
    FindFunctionCalls(CppModelManager *modelManager);

    // Find::IFindFilter
    virtual QString id() const { return QLatin1String("CppTools.Find.FunctionCalls"); }
    virtual QString name() const { return tr("Function calls"); }
    virtual bool isEnabled() const { return true; }
    virtual QKeySequence defaultShortcut() const { return QKeySequence(); }
    virtual void findAll(const QString &txt, QTextDocument::FindFlags findFlags);

protected Q_SLOTS:
    void displayResult(int);
    void searchFinished();
    void openEditor(const Find::SearchResultItem &item);

private:
    QPointer<CppModelManager> _modelManager;
    Find::SearchResultWindow *_resultWindow;
    QFutureWatcher<Core::Utils::FileSearchResult> m_watcher;
};

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
    void shutdown();
    CppModelManager *cppModelManager() { return m_modelManager; }
    QString correspondingHeaderOrSource(const QString &fileName) const;

private slots:
    void switchHeaderSource();

private:
    QString correspondingHeaderOrSourceI(const QString &fileName) const;
    QFileInfo findFile(const QDir &dir, const QString &name, const ProjectExplorer::Project *project) const;

    int m_context;
    CppModelManager *m_modelManager;
    CppCodeCompletion *m_completion;
    QSharedPointer<CppFileSettings> m_fileSettings;

    static CppToolsPlugin *m_instance;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPTOOLS_H
