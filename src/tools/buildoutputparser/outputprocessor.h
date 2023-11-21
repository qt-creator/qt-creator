// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QIODevice;
class QTextStream;
QT_END_NAMESPACE

namespace ProjectExplorer { class Task; }

enum CompilerType {
    CompilerTypeGcc,
    CompilerTypeClang,
    CompilerTypeMsvc
};

class CompilerOutputProcessor : public QObject
{
    Q_OBJECT
public:
    CompilerOutputProcessor(CompilerType compilerType, QIODevice &source);
     ~CompilerOutputProcessor();

    void start();
private:
    void handleTask(const ProjectExplorer::Task &task);

    const CompilerType m_compilerType;
    QIODevice &m_source;
    QTextStream * const m_ostream;
};
