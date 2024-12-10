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
// This file is a part of the QuickContainers library.
//
// \file    qcmAbstractContainer.h
// \author  benoit@destrat.io
// \date    2015 06 20
//-----------------------------------------------------------------------------

#pragma once

// Std headers
#include <memory>               // std::unique_ptr

// Qt headers
#include <QAbstractListModel>
#include <QPointer>

// QuickContainers headers
#include "qcmContainerModel.h"

namespace qcm { // ::qcm

/*! \brief Expose a consistent QAbstractModel for all QCM containers.
 *
 * \nosubgrouping
 */
class AbstractContainer : public QObject
{
    Q_OBJECT
public:
    explicit AbstractContainer(QObject* parent = nullptr) : QObject{parent} { /* Nil */ }
    virtual ~AbstractContainer() { /* _model destroyed in sub class */ }

    AbstractContainer(const AbstractContainer&) = delete;
    AbstractContainer& operator=(const AbstractContainer&) = delete;
    AbstractContainer(AbstractContainer&&) = delete;

    // WARNING: _model must have been checked on user side before calling theses fwd methods.
    template <typename... Args>         // std::forward<As>(a)...
    inline void    fwdBeginInsertRows(Args... args) noexcept { if (_model) _model->fwdBeginInsertRows(std::forward<Args>(args)...); }
    inline void    fwdEndInsertRows() noexcept { if (_model) _model->fwdEndInsertRows(); }
    inline void    fwdEmitLengthChanged() noexcept { if (_model) _model->fwdEmitLengthChanged(); }

    template <typename... Args>         // std::forward<As>(a)...
    inline void    fwdBeginRemoveRows(Args... args) noexcept { if (_model) _model->fwdBeginRemoveRows(std::forward<Args>(args)...); }
    inline void    fwdEndRemoveRows() noexcept { if (_model) _model->fwdEndRemoveRows(); }
    inline void    fwdBeginResetModel() noexcept { if (_model) _model->fwdBeginResetModel(); }
    inline void    fwdEndResetModel() noexcept { if (_model) _model->fwdEndResetModel(); }

public:
    Q_PROPERTY(ContainerModel*  model READ getModel CONSTANT FINAL)
    /*! \brief Return a Qt model for this container extended with a modification interface for the underlining container model from QML.
     *
     * \warning Underlying model is created \b synchronously on first \c model access, expect a quite slow first call (O(n), n beein container size).
     */
    inline ContainerModel*      getModel() noexcept {
        if (!_model)
            createModel();
        auto model = _model.data();
        QQmlEngine::setObjectOwnership(model, QQmlEngine::CppOwnership);
        return model;
    }
    //! Shortcut to getModel().
    inline ContainerModel*      model() const noexcept { return const_cast<AbstractContainer*>(this)->getModel(); }
protected:
    //! Create a concrete container model list reference for this abstract interface (called once).
    virtual void                createModel() { }
protected:
    mutable QPointer<ContainerModel>    _model;
};


} // ::qcm

QML_DECLARE_TYPE(qcm::AbstractContainer);
