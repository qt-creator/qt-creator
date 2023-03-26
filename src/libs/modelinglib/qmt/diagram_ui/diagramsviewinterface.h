// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qmt {

class MDiagram;
class DiagramSceneModel;

class DiagramsViewInterface
{
public:
    virtual ~DiagramsViewInterface() { }

    virtual void openDiagram(MDiagram *) = 0;
    virtual void closeDiagram(const MDiagram *) = 0;
    virtual void closeAllDiagrams() = 0;
    virtual void onDiagramRenamed(const MDiagram *) = 0;
};

} // namespace qmt
