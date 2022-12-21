// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

struct QString
{
    void append(char ch);
};

template <typename _Tp> struct QList {
    const _Tp &at(int index);
};

struct QStringList: public QList<QString> {};

int main()
{
    QStringList l;
    l.at(0).append('a'); 
}
