// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qmleventlocation.h"
#include <texteditor/textmark.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerViewManager;

class QmlProfilerTextMark : public TextEditor::TextMark
{
public:
    QmlProfilerTextMark(QmlProfilerViewManager *viewManager, int typeId,
                        const Utils::FilePath &fileName, int lineNumber);
    void addTypeId(int typeId);

    bool addToolTipContent(QLayout *target) const override;

private:
    QmlProfilerViewManager *m_viewManager;
    QVector<int> m_typeIds;
};

class QmlProfilerTextMarkModel : public QObject
{
public:
    QmlProfilerTextMarkModel(QObject *parent = nullptr);
    ~QmlProfilerTextMarkModel() override;

    void clear();
    void addTextMarkId(int typeId, const QmlEventLocation &location);
    void createMarks(QmlProfilerViewManager *viewManager, const QString &fileName);

    void showTextMarks();
    void hideTextMarks();

private:
    struct TextMarkId {
        int typeId;
        int lineNumber;
        int columnNumber;
    };

    QMultiHash<QString, TextMarkId> m_ids;
    QVector<QmlProfilerTextMark *> m_marks;
};

} // namespace Internal
} // namespace QmlProfiler
