#ifndef MULTIPLIERTESTER_H
#define MULTIPLIERTESTER_H

#include <chrono>
#include <iostream>

#include "matrix.h"
#include "simplematrixmultiplier.h"



/**
 * This class implements a tester for the multiplier. It calls a simple
 * implementation of multiply and the multi-threaded one, compares the
 * result, and the time spent by both implementation.
 */
template<class ThreadedMultiplierType>
class MultiplierTester
{
public:
    MultiplierTester() = default;

    void test(int matrixSize, int nbThreads, int nbBlocksPerRow)
    {
        using T = decltype(ThreadedMultiplierType::getElementType());

        SquareMatrix<T> A(matrixSize);
        SquareMatrix<T> B(matrixSize);
        SquareMatrix<T> C(matrixSize);
        SquareMatrix<T> C_ref(matrixSize);

        for (int i = 0; i < matrixSize; i++) {
            for (int j = 0; j < matrixSize; j++) {
                A.setElement(i, j, rand());
                B.setElement(i, j, rand());
                C.setElement(i, j, 0);
                C_ref.setElement(i, j, 0);
            }
        }

        SimpleMatrixMultiplier<T> multiplier;
        auto start = std::chrono::steady_clock::now();
        multiplier.multiply(A, B, C_ref);
        auto end = std::chrono::steady_clock::now();
        int64_t timeSimple = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        ThreadedMultiplierType threadedMultiplier(nbThreads, nbBlocksPerRow);
        start = std::chrono::steady_clock::now();
        threadedMultiplier.multiply(A, B, C);
        end = std::chrono::steady_clock::now();
        int64_t timeThreaded = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        C.compare(C_ref);

        if (timeThreaded == 0) {
            std::cout << "Time too short, try with a bigger matrix size" << std::endl;
        }
        else {
            double gain = static_cast<double>(timeSimple) / static_cast<double>(timeThreaded) * 100.0 - 100.0;
            std::cout << "Time gain: " << gain << " % " << std::endl;
        }
    }
};

#endif // MULTIPLIERTESTER_H
