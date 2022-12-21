// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "todoitemsscanner.h"

#include <utils/filepath.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

namespace Todo {
namespace Internal {

class QmlJsTodoItemsScanner : public TodoItemsScanner
{
public:
    explicit QmlJsTodoItemsScanner(const KeywordList &keywordList, QObject *parent = nullptr);

protected:
    bool shouldProcessFile(const Utils::FilePath &fileName);
    void scannerParamsChanged() override;

private:
    void documentUpdated(QmlJS::Document::Ptr doc);
    void processDocument(QmlJS::Document::Ptr doc);
};

}
}
