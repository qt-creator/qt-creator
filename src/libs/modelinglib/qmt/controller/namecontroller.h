// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include <QString>
#include <QStringList>

namespace qmt {

class QMT_EXPORT NameController : public QObject
{
private:
    explicit NameController(QObject *parent = nullptr);
    ~NameController() override;

public:
    static QString convertFileNameToElementName(const QString &fileName);
    static QString convertElementNameToBaseFileName(const QString &elementName);
    // TODO use Utils::FilePath instead
    static QString calcRelativePath(const QString &absoluteFileName, const QString &anchorPath);
    static QString calcElementNameSearchId(const QString &elementName);
    static QStringList buildElementsPath(const QString &filePath, bool ignoreLastFilePathPart);
    static bool parseClassName(const QString &fullClassName, QString *umlNamespace,
                               QString *className, QStringList *templateParameters);
};

} // namespace qmt
