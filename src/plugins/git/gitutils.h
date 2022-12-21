// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Git::Internal {

class Stash
{
public:
    void clear();
    bool parseStashLine(const QString &l);

    QString name;
    QString branch;
    QString message;
};

// Make QInputDialog  play nicely
bool inputText(QWidget *parent, const QString &title, const QString &prompt, QString *s);

// Version information following Qt convention
inline unsigned version(unsigned major, unsigned minor, unsigned patch)
{
    return (major << 16) + (minor << 8) + patch;
}

QString versionString(unsigned ver);

} // Git::Internal
