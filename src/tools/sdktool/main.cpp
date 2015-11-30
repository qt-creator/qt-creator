/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settings.h"

#include "operation.h"

#include "adddebuggeroperation.h"
#include "adddeviceoperation.h"
#include "addkeysoperation.h"
#include "addkitoperation.h"
#include "addqtoperation.h"
#include "addtoolchainoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmdebuggeroperation.h"
#include "rmdeviceoperation.h"
#include "rmkeysoperation.h"
#include "rmkitoperation.h"
#include "rmqtoperation.h"
#include "rmtoolchainoperation.h"

#include <iostream>

#include <QCoreApplication>
#include <QStringList>

void printHelp(const Operation *op)
{
    std::cout << "Qt Creator SDK setup tool." << std::endl;

    std::cout << "Help for operation " << qPrintable(op->name()) << std::endl;
    std::cout << std::endl;
    std::cout << qPrintable(op->argumentsHelpText());
    std::cout << std::endl;
}

void printHelp(const QList<Operation *> &operations)
{
    std::cout << "Qt Creator SDK setup tool." << std::endl;
    std::cout << "    Usage: " << qPrintable(qApp->arguments().at(0))
              << " <ARGS> <OPERATION> <OPERATION_ARGS>" << std::endl << std::endl;
    std::cout << "ARGS:" << std::endl;
    std::cout << "    --help|-h                Print this help text" << std::endl;
    std::cout << "    --sdkpath=PATH|-s PATH   Set the path to the SDK files" << std::endl << std::endl;

    std::cout << "OPERATION:" << std::endl;
    std::cout << "    One of:" << std::endl;
    foreach (const Operation *o, operations)
        std::cout << "        " << qPrintable(o->name()) << "\t\t" << qPrintable(o->helpText()) << std::endl;
    std::cout << std::endl;
    std::cout << "OPERATION_ARGS:" << std::endl;
    std::cout << "   use \"--help <OPERATION>\" to get help on the arguments required for an operation." << std::endl;

    std::cout << std::endl;
}

int parseArguments(const QStringList &args, Settings *s, const QList<Operation *> &operations)
{
    QStringList opArgs;
    int argCount = args.count();

    for (int i = 1; i < argCount; ++i) {
        const QString current = args[i];
        const QString next = (i + 1 < argCount) ? args[i + 1] : QString();

        if (!s->operation) {
            // help
            if (current == QLatin1String("-h") || current == QLatin1String("--help")) {
                if (!next.isEmpty()) {
                    foreach (Operation *o, operations) {
                        if (o->name() == next) {
                            printHelp(o);
                            return 0;
                        }
                    }
                }
                printHelp(operations);
                return 0;
            }

            // sdkpath
            if (current.startsWith(QLatin1String("--sdkpath="))) {
                s->sdkPath = Utils::FileName::fromString(current.mid(10));
                continue;
            }
            if (current == QLatin1String("-s")) {
                if (next.isNull()) {
                    std::cerr << "Missing argument to '-s'." << std::endl << std::endl;
                    printHelp(operations);
                    return 1;
                }
                s->sdkPath = Utils::FileName::fromString(next);
                ++i; // skip next;
                continue;
            }

            // operation
            foreach (Operation *o, operations) {
                if (o->name() == current) {
                    s->operation = o;
                    break;
                }
            }

            if (s->operation)
                continue;
        } else {
            opArgs << current;
            continue;
        }

        std::cerr << "Unknown parameter given." << std::endl << std::endl;
        printHelp(operations);
        return 1;
    }

    if (!s->operation) {
        std::cerr << "No operation requested." << std::endl << std::endl;
        printHelp(operations);
        return 1;
    }
    if (!s->operation->setArguments(opArgs)) {
        std::cerr << "Argument parsing failed." << std::endl << std::endl;
        printHelp(s->operation);
        s->operation = 0;
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    // Since 5.3, Qt by default aborts if the effective user id is different than the
    // real user id. However, in IFW on Mac we use setuid to 'elevate'
    // permissions if needed. This is considered safe because the user has to provide
    // the credentials manually - an attack would require at least access to the
    // user's environment.
    QCoreApplication::setSetuidAllowed(true);

    QCoreApplication a(argc, argv);

    Settings settings;

    QList<Operation *> operations;
    operations << new AddKeysOperation

               << new AddDebuggerOperation
               << new AddDeviceOperation
               << new AddQtOperation
               << new AddToolChainOperation

               << new AddKitOperation

               << new GetOperation
               << new RmKitOperation
               << new RmDebuggerOperation
               << new RmDeviceOperation
               << new RmKeysOperation
               << new RmQtOperation
               << new RmToolChainOperation

               << new FindKeyOperation
               << new FindValueOperation;

#ifdef WITH_TESTS
    if (argc == 2 && !strcmp(argv[1], "-test")) {
        std::cerr << std::endl << std::endl << "Starting tests..." << std::endl;
        int res = 0;
        foreach (Operation *o, operations)
            if (!o->test()) {
                std::cerr << "!!!! Test failed for: " << qPrintable(o->name()) << " !!!!" << std::endl;
                ++res;
            }
        std::cerr << "Tests done." << std::endl << std::endl;
        return res;
    }
#endif

    int result = parseArguments(a.arguments(), &settings, operations);
    if (!settings.operation)
        return result; // nothing to do:-)

    return settings.operation->execute();
}
