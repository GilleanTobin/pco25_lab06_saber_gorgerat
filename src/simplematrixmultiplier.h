#ifndef SIMPLEMATRIXMULTIPLIER_H
#define SIMPLEMATRIXMULTIPLIER_H

#include "abstractmatrixmultiplier.h"

/**
 * A simple implementation of the matrix multiplication.
 */
template<class T>
class SimpleMatrixMultiplier : public AbstractMatrixMultiplier<T>
{
public:
    void multiply(const SquareMatrix<T>& A, const SquareMatrix<T>& B, SquareMatrix<T>& C) override
    {
        for (int i = 0; i < A.size(); i++) {
            for (int j = 0; j < A.size(); j++) {
                T result = 0.0;
                for (int k = 0; k < A.size(); k++) {
                    result += A.element(k, j) * B.element(i, k);
                }
                C.setElement(i, j, result);
            }
        }
    }
};


#endif // SIMPLEMATRIXMULTIPLIER_H
