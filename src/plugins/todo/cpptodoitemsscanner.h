// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "todoitemsscanner.h"

#include <cppeditor/cppmodelmanager.h>

namespace Todo {
namespace Internal {

class CppTodoItemsScanner : public TodoItemsScanner
{
public:
    explicit CppTodoItemsScanner(const KeywordList &keywordList, QObject *parent = nullptr);

protected:
    void scannerParamsChanged() override;

private:
    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void processDocument(CPlusPlus::Document::Ptr doc);
};

}
}
