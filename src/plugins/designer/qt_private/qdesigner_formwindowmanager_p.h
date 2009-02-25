/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_FORMWINDOMANAGER_H
#define QDESIGNER_FORMWINDOMANAGER_H

#include "shared_global_p.h"
#include <QtDesigner/QDesignerFormWindowManagerInterface>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class PreviewManager;

//
// Convenience methods to manage form previews (ultimately forwarded to PreviewManager).
//
class QDESIGNER_SHARED_EXPORT QDesignerFormWindowManager
    : public QDesignerFormWindowManagerInterface
{
    Q_OBJECT
public:
    explicit QDesignerFormWindowManager(QObject *parent = 0);
    virtual ~QDesignerFormWindowManager();

    virtual QAction *actionDefaultPreview() const;
    virtual QActionGroup *actionGroupPreviewInStyle() const;
    virtual QAction *actionShowFormWindowSettingsDialog() const;

    virtual QPixmap createPreviewPixmap(QString *errorMessage) = 0;

    virtual PreviewManager *previewManager() const = 0;

Q_SIGNALS:
    void formWindowSettingsChanged(QDesignerFormWindowInterface *fw);

public Q_SLOTS:
    virtual void closeAllPreviews() = 0;

private:
    void *m_unused;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_FORMWINDOMANAGER_H
