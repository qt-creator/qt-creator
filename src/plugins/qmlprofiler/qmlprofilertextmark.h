/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
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
                        const QString &fileName, int lineNumber);
    void addTypeId(int typeId);

    void paintIcon(QPainter *painter, const QRect &rect) const override;
    void clicked() override;
    bool isClickable() const override { return true; }
    bool addToolTipContent(QLayout *target) const override;

private:
    QmlProfilerViewManager *m_viewManager;
    QVector<int> m_typeIds;
};

class QmlProfilerTextMarkModel : public QObject
{
public:
    QmlProfilerTextMarkModel(QObject *parent = nullptr);
    ~QmlProfilerTextMarkModel();

    void clear();
    void addTextMarkId(int typeId, const QmlEventLocation &location);
    void createMarks(QmlProfilerViewManager *viewManager, const QString &fileName);

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
