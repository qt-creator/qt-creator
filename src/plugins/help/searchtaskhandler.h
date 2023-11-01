// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/itaskhandler.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class SearchTaskHandler : public ProjectExplorer::ITaskHandler
{
    Q_OBJECT

public:
    bool canHandle(const ProjectExplorer::Task &task) const override;
    void handle(const ProjectExplorer::Task &task) override;
    QAction *createAction(QObject *parent) const override;

signals:
    void search(const QUrl &url);
};

} // namespace Internal
} // namespace Help
