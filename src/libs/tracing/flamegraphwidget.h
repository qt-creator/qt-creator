// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Timeline {

class FlameGraphWidgetPrivate;

class TRACING_EXPORT FlameGraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FlameGraphWidget(QAbstractItemModel *model, QWidget *parent = nullptr);
    ~FlameGraphWidget() override;

    QAbstractItemModel *model() const;

    // Mode selector: list of {sizeRole, displayName} pairs shown in the combo box.
    // The first entry is the default mode.
    void setSizeRoles(const QList<QPair<int, QString>> &roles);

    void setTypeIdRole(int role);
    void setSourceRoles(int fileRole, int lineRole, int columnRole = -1);
    void setSummaryRole(int role);

    // Details shown on click/hover, emitted via detailsChanged()/detailsCleared().
    void setDetailsTitleRole(int role);
    void setDetailsRoles(const QList<QPair<int, QString>> &roles);
    void setNoteRole(int role);

    // Text shown for "others" aggregate nodes (nodes below the size threshold)
    void setOthersText(const QString &text);

    void selectByTypeId(int typeId);
    void resetRoot();
    bool isZoomed() const;

signals:
    void typeSelected(int typeId);
    void gotoSourceLocation(const QString &file, int line, int column);
    void detailsChanged(const QString &title, const QList<QPair<QString, QString>> &content);
    void detailsCleared();

private:
    FlameGraphWidgetPrivate *d;
};

} // namespace Timeline
