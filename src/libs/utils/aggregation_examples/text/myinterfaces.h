// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <aggregate.h>

#include <QString>

class IComboEntry : public QObject
{
    Q_OBJECT

public:
    IComboEntry(QString title) : m_title(title) {}
    virtual ~IComboEntry() {}
    QString title() const { return m_title; }

private:
    QString m_title;
};

class IText1 : public QObject
{
    Q_OBJECT

public:
    IText1(QString text) : m_text(text) {}
    virtual ~IText1() {}
    QString text() const { return m_text; }

private:
    QString m_text;
};

class IText2 : public QObject
{
    Q_OBJECT

public:
    IText2(QString text) : m_text(text) {}
    QString text() const { return m_text; }

private:
    QString m_text;
};

class IText3 : public QObject
{
    Q_OBJECT

public:
    IText3(QString text) : m_text(text) {}
    virtual ~IText3() {}
    QString text() const { return m_text; }

private:
    QString m_text;
};
