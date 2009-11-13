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

#include <AST.h>
#include <ASTVisitor.h>
#include <ASTMatcher.h>
#include <Control.h>
#include <Scope.h>
#include <Semantic.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <CoreTypes.h>

#include <QFile>
#include <QList>
#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include <QTime>
#include <QtDebug>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace CPlusPlus;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    const QString appName = args.first();
    args.removeFirst();

    foreach (const QString &arg, args) {
        if (arg == QLatin1String("--help")) {
            const QFileInfo appInfo(appName);
            const QByteArray appFileName = QFile::encodeName(appInfo.fileName());

            printf("Usage: %s [options]\n"
                   "  --help                    Display this information\n",
                   appFileName.constData());

            return EXIT_SUCCESS;
        }
    }

    QFile in;
    if (! in.open(stdin, QFile::ReadOnly))
        return EXIT_FAILURE;

    const QByteArray source = in.readAll();

    Control control;
    StringLiteral *fileId = control.findOrInsertStringLiteral("<stdin>");
    TranslationUnit unit(&control, fileId);
    unit.setObjCEnabled(true);
    unit.setSource(source.constData(), source.size());
    unit.parse();
    if (! unit.ast())
        return EXIT_FAILURE;

    TranslationUnitAST *ast = unit.ast()->asTranslationUnit();
    Q_ASSERT(ast != 0);

    Namespace *globalNamespace = control.newNamespace(0, 0);
    Semantic sem(&control);
    for (DeclarationListAST *it = ast->declaration_list; it; it = it->next) {
        DeclarationAST *declaration = it->value;
        sem.check(declaration, globalNamespace->members());
    }

    return EXIT_SUCCESS;
}
