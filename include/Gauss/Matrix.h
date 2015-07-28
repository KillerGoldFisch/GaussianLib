/*
 * Matrix.h
 * 
 * This file is part of the "GaussianLib" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __GS_MATRIX_H__
#define __GS_MATRIX_H__


#include "Real.h"
#include "Assert.h"
#include "Macros.h"
#include "Tags.h"

#include <cmath>
#include <cstring>
#include <algorithm>


namespace Gs
{


#define __GS_ASSERT_NxN_MATRIX__ \
    static_assert(Rows == Cols, __GS_FILE_LINE__ "function can only be used with NxN matrices")

#ifdef GS_ROW_MAJOR_STORAGE
#   define __GS_FOREACH_ROW_COL__(r, c)         \
        for (std::size_t r = 0; r < Rows; ++r)  \
        for (std::size_t c = 0; c < Cols; ++c)
#else
#   define __GS_FOREACH_ROW_COL__(r, c)         \
        for (std::size_t c = 0; c < Cols; ++c)  \
        for (std::size_t r = 0; r < Rows; ++r)
#endif

/**
\brief Base matrix class.
\tparam T Specifies the data type of the matrix components.
This should be a primitive data type such as float, double, int etc.
\remarks The macro GS_ROW_MAJOR_STORAGE can be defined, to use row-major storage layout.
By default column-major storage layout is used.
*/
template <typename T, std::size_t Rows, std::size_t Cols> class Matrix
{
    
    public:
        
        static_assert(Rows*Cols > 0, "matrices must consist of at least 1x1 elements");

        static const std::size_t rows       = Rows;
        static const std::size_t columns    = Cols;
        static const std::size_t elements   = Rows*Cols;

        using ThisType = Matrix<T, Rows, Cols>;

        //! Transposed matrix type, i.e. NxM becomes MxN.
        using TransposedType = Matrix<T, Cols, Rows>;

        class Initializer
        {
            
            public:
                
                Initializer(ThisType& matrix) :
                    matrix_ ( matrix ),
                    element_( 0      )
                {
                }

                Initializer& operator , (const T& nextValue)
                {
                    matrix_(element_ / Cols, element_ % Cols) = nextValue;
                    ++element_;
                    return *this;
                }

            private:

                ThisType&   matrix_;
                std::size_t element_;

        };

        Matrix()
        {
            #ifndef GS_DISABLE_AUTO_INIT
            Reset();
            #endif
        }

        Matrix(const ThisType& rhs)
        {
            *this = rhs;
        }

        Matrix(UninitializeTag)
        {
            // do nothing
        }

        T& operator () (std::size_t row, std::size_t col)
        {
            GS_ASSERT(row < Rows);
            GS_ASSERT(col < Cols);
            #ifdef GS_ROW_MAJOR_STORAGE
            return m_[row*Cols + col];
            #else
            return m_[col*Rows + row];
            #endif
        }

        const T& operator () (std::size_t row, std::size_t col) const
        {
            GS_ASSERT(row < Rows);
            GS_ASSERT(col < Cols);
            #ifdef GS_ROW_MAJOR_STORAGE
            return m_[row*Cols + col];
            #else
            return m_[col*Rows + row];
            #endif
        }

        T& operator [] (std::size_t element)
        {
            GS_ASSERT(element < ThisType::elements);
            return m_[element];
        }

        const T& operator [] (std::size_t element) const
        {
            GS_ASSERT(element < ThisType::elements);
            return m_[element];
        }

        ThisType& operator += (const ThisType& rhs)
        {
            for (std::size_t i = 0; i < ThisType::elements; ++i)
                m_[i] += rhs.m_[i];
            return *this;
        }

        ThisType& operator -= (const ThisType& rhs)
        {
            for (std::size_t i = 0; i < ThisType::elements; ++i)
                m_[i] -= rhs.m_[i];
            return *this;
        }

        ThisType& operator *= (const ThisType& rhs)
        {
            __GS_ASSERT_NxN_MATRIX__;
            *this = (*this * rhs);
            return *this;
        }

        ThisType& operator *= (const T& rhs)
        {
            for (std::size_t i = 0; i < ThisType::elements; ++i)
                m_[i] *= rhs;
            return *this;
        }

        ThisType& operator = (const ThisType& rhs)
        {
            for (std::size_t i = 0; i < ThisType::elements; ++i)
                m_[i] = rhs.m_[i];
            return *this;
        }

        void Reset()
        {
            for (std::size_t i = 0; i < ThisType::elements; ++i)
                m_[i] = T(0);
        }

        void LoadIdentity()
        {
            __GS_ASSERT_NxN_MATRIX__;
            __GS_FOREACH_ROW_COL__(r, c)
            {
                (*this)(r, c) = (r == c ? T(1) : T(0));
            }
        }

        static ThisType Identity()
        {
            ThisType result;
            result.LoadIdentity();
            return result;
        }

        TransposedType Transposed() const
        {
            TransposedType result;

            __GS_FOREACH_ROW_COL__(r, c)
            {
                result(c, r) = (*this)(r, c);
            }

            return result;
        }

        void Transpose()
        {
            __GS_ASSERT_NxN_MATRIX__;

            for (std::size_t i = 0; i + 1 < Cols; ++i)
            {
                for (std::size_t j = 1; j + i < Cols; ++j)
                {
                    std::swap(
                        m_[i*(Cols + 1) + j],
                        m_[(j + i)*Cols + i]
                    );
                }
            }
        }

        T Determinant() const
        {
            return Gs::Determinant(*this);
        }

        /**
        Returns the trace of this matrix: M(0, 0) + M(1, 1) + ... + M(N - 1, N - 1).
        \note This can only be used for squared matrices!
        */
        T Trace() const
        {
            static_assert(Rows == Cols, "traces can only be computed for squared matrices");
            
            T trace = T(0);

            for (std::size_t i = 0; i < Rows; ++i)
                trace += (*this)(i, i);

            return trace;
        }

        Matrix<T, Rows, Cols> Inverse() const
        {
            Matrix<T, Rows, Cols> inv{ *this };
            inv.MakeInverse();
            return inv;
        }

        bool MakeInverse()
        {
            Matrix<T, Rows, Cols> in{ *this };
            return Gs::Inverse(*this, in);
        }

        /**
        \brief Rotates this matrix around the specified axis.
        \see MakeFreeRotation
        */
        template <template <typename> class Vec>
        void RotateFree(const Vec<T>& axis, const T& angle)
        {
            auto rotation = ThisType::Identity();
            Gs::MakeFreeRotation(rotation, axis, angle);
            *this *= rotation;
        }

        //! Returns a pointer to the first element of this matrix.
        T* Ptr()
        {
            return &(m_[0]);
        }

        //! Returns a constant pointer to the first element of this matrix.
        const T* Ptr() const
        {
            return &(m_[0]);
        }

    private:
        
        T m_[ThisType::elements];

};

#undef __GS_ASSERT_NxN_MATRIX__
#undef __GS_FOREACH_ROW_COL__


/* --- Global Operators --- */

template <typename T, std::size_t Rows, std::size_t Cols>
Matrix<T, Rows, Cols> operator + (const Matrix<T, Rows, Cols>& lhs, const Matrix<T, Rows, Cols>& rhs)
{
    auto result = lhs;
    result += rhs;
    return result;
}

template <typename T, std::size_t Rows, std::size_t Cols>
Matrix<T, Rows, Cols> operator - (const Matrix<T, Rows, Cols>& lhs, const Matrix<T, Rows, Cols>& rhs)
{
    auto result = lhs;
    result -= rhs;
    return result;
}

template <typename T, std::size_t Rows, std::size_t Cols>
Matrix<T, Rows, Cols> operator * (const Matrix<T, Rows, Cols>& lhs, const T& rhs)
{
    auto result = lhs;
    result *= rhs;
    return result;
}

template <typename T, std::size_t Rows, std::size_t Cols>
Matrix<T, Rows, Cols> operator * (const T& lhs, const Matrix<T, Rows, Cols>& rhs)
{
    auto result = rhs;
    result *= lhs;
    return result;
}

template <typename T, std::size_t Rows, std::size_t ColsRows, std::size_t Cols>
Matrix<T, Rows, Cols> operator * (const Matrix<T, Rows, ColsRows>& lhs, const Matrix<T, ColsRows, Cols>& rhs)
{
    Matrix<T, Rows, Cols> result(UninitializeTag{});

    for (std::size_t r = 0; r < Rows; ++r)
    {
        for (std::size_t c = 0; c < Cols; ++c)
        {
            result(r, c) = T(0);
            for (std::size_t i = 0; i < ColsRows; ++i)
                result(r, c) += lhs(r, i)*rhs(i, c);
        }
    }

    return result;
}

template <typename T, typename I, std::size_t Rows, std::size_t Cols>
typename Matrix<T, Rows, Cols>::Initializer operator << (Matrix<T, Rows, Cols>& matrix, const I& firstValue)
{
    typename Matrix<T, Rows, Cols>::Initializer initializer(matrix);
    initializer , static_cast<T>(firstValue);
    return initializer;
}


/* --- Type Alias --- */

#define __GS_DEF_MATRIX_TYPES_MxN__(m, n)                           \
    template <typename T> using Matrix##m##n##T = Matrix<T, m, n>;  \
    using Matrix##m##n      = Matrix##m##n##T<Real>;                \
    using Matrix##m##n##f   = Matrix##m##n##T<float>;               \
    using Matrix##m##n##d   = Matrix##m##n##T<double>;              \
    using Matrix##m##n##i   = Matrix##m##n##T<int>;                 \
    using Matrix##m##n##ui  = Matrix##m##n##T<unsigned int>;        \
    using Matrix##m##n##b   = Matrix##m##n##T<char>;                \
    using Matrix##m##n##ub  = Matrix##m##n##T<unsigned char>

__GS_DEF_MATRIX_TYPES_MxN__(3, 4);
__GS_DEF_MATRIX_TYPES_MxN__(4, 3);

#define __GS_DEF_MATRIX_TYPES_NxN__(n)                          \
    template <typename T> using Matrix##n##T = Matrix<T, n, n>; \
    using Matrix##n     = Matrix##n##T<Real>;                   \
    using Matrix##n##f  = Matrix##n##T<float>;                  \
    using Matrix##n##d  = Matrix##n##T<double>;                 \
    using Matrix##n##i  = Matrix##n##T<int>;                    \
    using Matrix##n##ui = Matrix##n##T<unsigned int>;           \
    using Matrix##n##b  = Matrix##n##T<char>;                   \
    using Matrix##n##ub = Matrix##n##T<unsigned char>

__GS_DEF_MATRIX_TYPES_NxN__(2);
__GS_DEF_MATRIX_TYPES_NxN__(3);
__GS_DEF_MATRIX_TYPES_NxN__(4);

#undef __GS_DEF_MATRIX_TYPES_MxN__
#undef __GS_DEF_MATRIX_TYPES_NxN__


} // /namespace Gs


#endif



// ================================================================================
