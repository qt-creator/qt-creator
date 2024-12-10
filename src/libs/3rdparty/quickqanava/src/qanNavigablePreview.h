/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	qanNavigablePreview.h
// \author	benoit@destrat.io
// \date	2017 06 02
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>

// QuickQanava headers
#include "./qanNavigable.h"

namespace qan { // ::qan

/*! \brief Bastract interface for reduced preview and navigation for qan::Navigable.
 *
 * See Qan.NavigablePreview component for more documention.
 */
class NavigablePreview : public QQuickItem
{
    /*! \name NavigablePreview Object Management *///--------------------------
    //@{
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractNavigablePreview)

public:
    explicit NavigablePreview(QQuickItem* parent = nullptr);
    virtual ~NavigablePreview() = default;
    NavigablePreview(const NavigablePreview&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Preview Management *///------------------------------------------
    //@{
public:
    /*! \brief Source qan::Navigable.
     *
     * \warning Can be nullptr.
     */
    Q_PROPERTY(qan::Navigable* source READ getSource WRITE setSource NOTIFY sourceChanged FINAL)
    //! \copydoc source
    inline qan::Navigable*      getSource() const noexcept { return _source.data(); }
    //! \copydoc source
    void                        setSource(qan::Navigable* source) noexcept;
private:
    //! \copydoc source
    QPointer<qan::Navigable>    _source;
signals:
    //! \copydoc source
    void                        sourceChanged();

protected:
    //! Return union of rects \c a and \c b.
    Q_INVOKABLE QRectF  rectUnion(QRectF a, QRectF b) const;

signals:
    /*! \brief Emitted whenever the preview visible window position or size change.
     *
     * \warning \c visibleWindowRect is scaled to (0,1) according to original navigable
     * \c contentRect.
     * \arg visibleWindowRect visible window rectangle in navigable \c containerItem CS.
     * \arg navigableZoom zoom in the underling navigable when change occurs.
     */
    void        visibleWindowChanged(QRectF visibleWindowRect, qreal navigableZoom);
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::NavigablePreview)
