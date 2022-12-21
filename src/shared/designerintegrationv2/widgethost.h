// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QScrollArea>

QT_FORWARD_DECLARE_CLASS(QDesignerFormWindowInterface)

namespace SharedTools {

namespace Internal { class FormResizer; }

/* A scroll area that embeds a Designer form window */

class WidgetHost : public QScrollArea
{
    Q_OBJECT
public:
    explicit WidgetHost(QWidget *parent = 0, QDesignerFormWindowInterface *formWindow = 0);
    virtual ~WidgetHost();
    // Show handles if active and main container is selected.
    void updateFormWindowSelectionHandles(bool active);

    inline QDesignerFormWindowInterface *formWindow() const { return m_formWindow; }

    QWidget *integrationContainer() const;

protected:
    void setFormWindow(QDesignerFormWindowInterface *fw);

signals:
    void formWindowSizeChanged(int, int);

private slots:
    void fwSizeWasChanged(const QRect &, const QRect &);

private:
    QSize formWindowSize() const;

    QDesignerFormWindowInterface *m_formWindow = nullptr;
    Internal::FormResizer *m_formResizer;
    QSize m_oldFakeWidgetSize;
};

} // namespace SharedTools
