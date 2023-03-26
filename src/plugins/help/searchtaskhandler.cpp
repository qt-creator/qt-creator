// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchtaskhandler.h"

#include "helptr.h"

#include <projectexplorer/task.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QUrl>

using namespace Help::Internal;

bool SearchTaskHandler::canHandle(const ProjectExplorer::Task &task) const
{
    return !task.summary.isEmpty();
}

void SearchTaskHandler::handle(const ProjectExplorer::Task &task)
{
    emit search(QUrl("https://www.google.com/search?q=" + task.summary));
}

QAction *SearchTaskHandler::createAction(QObject *parent) const
{
    return new QAction(Tr::tr("Get Help Online"), parent);
}
