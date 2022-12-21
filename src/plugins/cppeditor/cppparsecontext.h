// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cpptoolsreuse.h"

#include <QAbstractListModel>
#include <QComboBox>

namespace CppEditor {
namespace Internal {

class ParseContextModel : public QAbstractListModel
{
    Q_OBJECT

public:
    void update(const ProjectPartInfo &projectPartInfo);

    void setPreferred(int index);
    void clearPreferred();

    int currentIndex() const;
    bool isCurrentPreferred() const;

    QString currentId() const;
    QString currentToolTip() const;

    bool areMultipleAvailable() const;

signals:
    void updated(bool areMultipleAvailable);
    void preferredParseContextChanged(const QString &id);

private:
    void reset(const ProjectPartInfo &projectPartInfo);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    ProjectPartInfo::Hints m_hints;
    QList<ProjectPart::ConstPtr> m_projectParts;
    int m_currentIndex = -1;
};

class ParseContextWidget : public QComboBox
{
    Q_OBJECT

public:
    ParseContextWidget(ParseContextModel &parseContextModel, QWidget *parent);
    void syncToModel();

    QSize minimumSizeHint() const final;

private:
    ParseContextModel &m_parseContextModel;
    QAction *m_clearPreferredAction = nullptr;
};

} // namespace Internal
} // namespace CppEditor
