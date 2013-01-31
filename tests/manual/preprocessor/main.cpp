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

#include <PreprocessorEnvironment.h>
#include <PreprocessorClient.h>
#include <pp.h>

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QDir>
#include <QDebug>

#include <iostream>

using namespace CPlusPlus;

class MakeDepend: public Client
{
    Environment *env;
    QList<QDir> systemDirs;
    QStringList included;

public:
    MakeDepend(Environment *env)
        : env(env)
    { }

    QStringList includedFiles() const
    { return included; }

    void addSystemDir(const QDir &dir)
    { systemDirs.append(dir); }

    void addSystemDir(const QString &path)
    { systemDirs.append(QDir(path)); }

    virtual void macroAdded(const Macro &)
    { }

    void addInclude(const QString &absoluteFilePath)
    { included.append(absoluteFilePath); }

    virtual void sourceNeeded(QString &fileName, IncludeType mode, unsigned)
    {
        const QString currentFile = env->currentFile;

        if (mode == IncludeLocal) {
            const QFileInfo currentFileInfo(currentFile);
            const QDir dir = currentFileInfo.dir();

            // ### cleanup
            QFileInfo fileInfo(dir, fileName);
            if (fileInfo.exists()) {
                addInclude(fileInfo.absoluteFilePath());
                return;
            }
        }

        foreach (const QDir &dir, systemDirs) {
            QFileInfo fileInfo(dir, fileName);
            if (fileInfo.exists() && fileInfo.isFile()) {
                addInclude(fileInfo.absoluteFilePath());
                return;
            }
        }

#ifdef PP_WITH_DIAGNOSTICS
        std::cerr << qPrintable(currentFile) << ':' << line << ": error: "
                << qPrintable(fileName) << ": No such file or directory" << std::endl;
#endif
    }

    virtual void passedMacroDefinitionCheck(unsigned, unsigned, const Macro &)
    { }

    virtual void failedMacroDefinitionCheck(unsigned, const ByteArrayRef &)
    { }

    virtual void notifyMacroReference(unsigned, unsigned, const Macro &)
    { }

    virtual void startExpandingMacro(unsigned, unsigned, const Macro &,
                                     const QVector<MacroArgumentReference> &)
    { }

    virtual void stopExpandingMacro(unsigned, const Macro &)
    { }

    virtual void startSkippingBlocks(unsigned)
    { }

    virtual void stopSkippingBlocks(unsigned)
    { }

    virtual void sourceNeeded(unsigned, QString &, IncludeType)
    { }
};

int make_depend(QCoreApplication *app);


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    args.removeFirst();

    foreach (const QString &fileName, args) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            QTextStream in(&file);
            const QString source = in.readAll();

            Environment env;
            Preprocessor pp(/*client=*/ 0, &env);
            const QByteArray preprocessedCode = pp.run(fileName, source);
            std::cout << preprocessedCode.constData();
        }
    }
}

int make_depend(QCoreApplication *app)
{
    QStringList todo = app->arguments();
    todo.removeFirst();

    if (todo.isEmpty())
        todo.append(QString::fromLocal8Bit(qgetenv("QTDIR")) + QLatin1String("/include/QtCore/QtCore"));

    QMap<QString, QStringList> processed;

    while (! todo.isEmpty()) {
        const QString fn = todo.takeFirst();

        if (processed.contains(fn))
            continue;

        QStringList deps;

        QFile file(fn);
        if (file.open(QFile::ReadOnly)) {
            // ### we should QTextStream here.
            const QByteArray code = file.readAll();

            Environment env;
            MakeDepend client(&env);
            client.addSystemDir(QString::fromLocal8Bit(qgetenv("QTDIR")) + QLatin1String("/include"));
            Preprocessor preproc(&client, &env);
            preproc.run(fn, code, /*result = */ 0);
            deps = client.includedFiles();
            todo += deps;
        }

        processed.insert(fn, deps);
    }

    QMapIterator<QString, QStringList> it(processed);
    while (it.hasNext()) {
        it.next();

        if (it.value().isEmpty())
            continue; // no deps, nothing to do.

        std::cout << qPrintable(it.key()) << ": \\\n  " << qPrintable(it.value().join(QLatin1String(" \\\n  ")))
                << std::endl << std::endl;
    }

    return 0;
}
