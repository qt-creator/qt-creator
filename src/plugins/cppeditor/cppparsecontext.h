/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <cpptools/cpptools_utils.h>

#include <QAbstractListModel>
#include <QComboBox>

namespace CppEditor {
namespace Internal {

class ParseContextModel : public QAbstractListModel
{
    Q_OBJECT

public:
    void update(const CppTools::ProjectPartInfo &projectPartInfo);

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
    void reset(const CppTools::ProjectPartInfo &projectPartInfo);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    CppTools::ProjectPartInfo::Hints m_hints;
    QList<CppTools::ProjectPart::Ptr> m_projectParts;
    int m_currentIndex = -1;
};

class ParseContextWidget : public QComboBox
{
    Q_OBJECT

public:
    ParseContextWidget(ParseContextModel &parseContextModel, QWidget *parent);
    void syncToModel();

private:
    ParseContextModel &m_parseContextModel;
    QAction *m_clearPreferredAction = nullptr;
};

} // namespace Internal
} // namespace CppEditor
