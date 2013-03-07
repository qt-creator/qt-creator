/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BARDESCRIPTOREDITORWIDGET_H
#define QNX_INTERNAL_BARDESCRIPTOREDITORWIDGET_H

#include "bardescriptordocument.h"

#include <QStackedWidget>

namespace Core {
class IEditor;
}

namespace ProjectExplorer {
class PanelsWidget;
}

namespace TextEditor {
class PlainTextEditorWidget;
}

namespace Qnx {
namespace Internal {

class BarDescriptorEditor;
class BarDescriptorEditorEntryPointWidget;
class BarDescriptorEditorPackageInformationWidget;
class BarDescriptorEditorAuthorInformationWidget;
class BarDescriptorEditorGeneralWidget;
class BarDescriptorEditorPermissionsWidget;
class BarDescriptorEditorEnvironmentWidget;
class BarDescriptorEditorAssetsWidget;

class BarDescriptorEditorWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit BarDescriptorEditorWidget(QWidget *parent = 0);

    Core::IEditor *editor() const;

    BarDescriptorEditorEntryPointWidget *entryPointWidget() const;
    BarDescriptorEditorPackageInformationWidget *packageInformationWidget() const;
    BarDescriptorEditorAuthorInformationWidget *authorInformationWidget() const;

    BarDescriptorEditorGeneralWidget *generalWidget() const;
    BarDescriptorEditorPermissionsWidget *permissionsWidget() const;
    BarDescriptorEditorEnvironmentWidget *environmentWidget() const;

    BarDescriptorEditorAssetsWidget *assetsWidget() const;

    QString xmlSource() const;
    void setXmlSource(const QString &xmlSource);

    bool isDirty() const;
    void clear();

public slots:
    void setDirty(bool dirty = true);

signals:
    void changed();

private:
    BarDescriptorEditor *createEditor();

    void initGeneralPage();
    void initApplicationPage();
    void initAssetsPage();
    void initSourcePage();
    void initPanelSize(ProjectExplorer::PanelsWidget *panelsWidget);

    mutable Core::IEditor *m_editor;

    bool m_dirty;

    // New UI
    BarDescriptorEditorEntryPointWidget *m_entryPointWidget;
    BarDescriptorEditorPackageInformationWidget *m_packageInformationWidget;
    BarDescriptorEditorAuthorInformationWidget *m_authorInformationWidget;

    BarDescriptorEditorGeneralWidget *m_generalWidget;
    BarDescriptorEditorPermissionsWidget *m_permissionsWidget;
    BarDescriptorEditorEnvironmentWidget *m_environmentWidget;

    BarDescriptorEditorAssetsWidget *m_assetsWidget;

    TextEditor::PlainTextEditorWidget *m_xmlSourceWidget;
};


} // namespace Internal
} // namespace Qnx
#endif // QNX_INTERNAL_BARDESCRIPTOREDITORWIDGET_H
