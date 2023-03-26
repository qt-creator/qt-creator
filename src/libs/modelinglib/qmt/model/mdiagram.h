// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mobject.h"

#include <QHash>
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
    DElement *findDelegate(const Uid &modelUid) const;
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
    QHash<Uid, DElement *> m_elementMap;
    QHash<Uid, DElement *> m_modelUid2ElementMap;
    QDateTime m_lastModified;
    QString m_toolbarId;
};

} // namespace qmt

Q_DECLARE_METATYPE(qmt::MDiagram *)
Q_DECLARE_METATYPE(const qmt::MDiagram *)
