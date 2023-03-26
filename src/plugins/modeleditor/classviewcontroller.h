// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <cplusplus/CppDocument.h>

namespace ModelEditor {
namespace Internal {

class ClassViewController :
        public QObject
{
    Q_OBJECT

public:
    explicit ClassViewController(QObject *parent = nullptr);
    ~ClassViewController() = default;

    QSet<QString> findClassDeclarations(const Utils::FilePath &filePath, int line = -1, int column = -1);

private:
    void appendClassDeclarationsFromDocument(CPlusPlus::Document::Ptr document, int line, int column,
                                             QSet<QString> *classNames);
    void appendClassDeclarationsFromSymbol(CPlusPlus::Symbol *symbol, int line, int column, QSet<QString> *classNames);
};

} // namespace Internal
} // namespace ModelEditor
