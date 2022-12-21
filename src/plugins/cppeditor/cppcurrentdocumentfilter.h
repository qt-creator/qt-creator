// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "searchsymbols.h"

#include <coreplugin/locator/ilocatorfilter.h>

namespace Core { class IEditor; }

namespace CppEditor {

class CppModelManager;

namespace Internal {

class CppCurrentDocumentFilter : public  Core::ILocatorFilter
{
    Q_OBJECT

public:
    explicit CppCurrentDocumentFilter(CppModelManager *manager);
    ~CppCurrentDocumentFilter() override = default;

    void makeAuxiliary();

    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;

private:
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void onCurrentEditorChanged(Core::IEditor *currentEditor);
    void onEditorAboutToClose(Core::IEditor *currentEditor);

    QList<IndexItem::Ptr> itemsOfCurrentDocument();

    CppModelManager * m_modelManager;
    SearchSymbols search;

    mutable QMutex m_mutex;
    Utils::FilePath m_currentFileName;
    QList<IndexItem::Ptr> m_itemsOfCurrentDoc;
};

} // namespace Internal
} // namespace CppEditor
