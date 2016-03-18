/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "mobject.h"

#include <QDateTime>

namespace qmt {

class DElement;

class QMT_EXPORT MDiagram : public MObject
{
public:
    MDiagram();
    MDiagram(const MDiagram &rhs);
    ~MDiagram() override;

    MDiagram &operator=(const MDiagram &rhs);

    const QList<DElement *> &diagramElements() const { return m_elements; }
    DElement *findDiagramElement(const Uid &key) const;
    void setDiagramElements(const QList<DElement *> &elements);

    void addDiagramElement(DElement *element);
    void insertDiagramElement(int beforeElement, DElement *element);
    void removeDiagramElement(int index);
    void removeDiagramElement(DElement *element);

    QDateTime lastModified() const { return m_lastModified; }
    void setLastModified(const QDateTime &lastModified);
    void setLastModifiedToNow();

    QString toolbarId() const { return m_toolbarId; }
    void setToolbarId(const QString &toolbarId);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;

private:
    QList<DElement *> m_elements;
    QDateTime m_lastModified;
    QString m_toolbarId;
};

} // namespace qmt
