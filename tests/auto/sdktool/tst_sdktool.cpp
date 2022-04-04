/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QtTest>

#ifndef WITH_TESTS
#define WITH_TESTS
#endif

#include "addabiflavor.h"
#include "addcmakeoperation.h"
#include "adddebuggeroperation.h"
#include "adddeviceoperation.h"
#include "addkeysoperation.h"
#include "addqtoperation.h"
#include "addtoolchainoperation.h"
#include "addvalueoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmcmakeoperation.h"
#include "rmdebuggeroperation.h"
#include "rmdeviceoperation.h"
#include "rmkeysoperation.h"
#include "rmkitoperation.h"
#include "rmqtoperation.h"
#include "rmtoolchainoperation.h"
#include "addkitoperation.h"

class SdktoolTest : public QObject
{
    Q_OBJECT

private slots:
    void test_AddAbiFlavorOperation() { AddAbiFlavor::unittest(); }
    void test_AddCMakeOperation() { AddCMakeOperation::unittest(); }
    void test_AddDebuggerOperation() { AddDebuggerOperation::unittest(); }
    void test_AddDeviceOperation() { AddDeviceOperation::unittest(); }
    void test_AddKeysOperation() { AddKeysOperation::unittest(); }
    void test_AddKitOperation() { AddKitOperation::unittest(); }
    void test_AddQtOperation() { AddQtOperation::unittest(); }
    void test_AddToolchainOperation() { AddToolChainOperation::unittest(); }
    void test_AddValueOperation() { AddValueOperation::unittest(); }
    void test_FindKeyOperation() { FindKeyOperation::unittest(); }
    void test_FindValueOperation() { FindValueOperation::unittest(); }
    void test_GetOperation() { GetOperation::unittest(); }
    void test_RmCMakeOperation() { RmCMakeOperation::unittest(); }
    void test_RmDebuggerOperation() { RmDebuggerOperation::unittest(); }
    void test_RmDeviceOperation() { RmDeviceOperation::unittest(); }
    void test_RmKeysOperation() { RmKeysOperation::unittest(); }
    void test_RmKitOperation() { RmKitOperation::unittest(); }
    void test_RmQtOperation() { RmQtOperation::unittest(); }
    void test_RmToolChainOperation() { RmToolChainOperation::unittest(); }
};

QTEST_MAIN(SdktoolTest)

#include "tst_sdktool.moc"
