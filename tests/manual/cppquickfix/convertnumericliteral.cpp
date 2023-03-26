// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

int main()
{
    // standard case
    199;
    074;
    0x856A;
    // with type specifier
    199LL;
    074L;
    0xFA0Bu;
    // uppercase X
    0X856A;
    // negative values
    -199;
    -017;
    // not integer, do nothing
    298.3;
    // ignore invalid octal
    0783;
    0; // border case, only hex<->decimal
}
