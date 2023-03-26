// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mybackendobject.h"

MyBackendObject::MyBackendObject(QObject *parent) : QObject(parent)
{

}

bool MyBackendObject::test() const
{
    return false;
}

void MyBackendObject::setTest(bool)
{

}
