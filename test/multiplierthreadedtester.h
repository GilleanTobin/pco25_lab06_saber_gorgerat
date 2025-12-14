#ifndef MULTIPLIERTHREADEDTESTER_H
#define MULTIPLIERTHREADEDTESTER_H

#include <chrono>
#include <iostream>
#include <memory>

#include <pcosynchro/pcothread.h>

#include "matrix.h"
#include "simplematrixmultiplier.h"


template<class ThreadedMultiplierType>
void test_int(int matrixSize, int nbBlocksPerRow, ThreadedMultiplierType* threadedMultiplier)
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

    start = std::chrono::steady_clock::now();
    threadedMultiplier->multiply(A, B, C, nbBlocksPerRow);
    end = std::chrono::steady_clock::now();
    int64_t timeThreaded = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    C.compare(C_ref);

    if (timeThreaded == 0) {
        std::cout << "Time too short, try with a bigger matrix size" << std::endl;
    }
    else {
        double gain = static_cast<double>(timeSimple) / static_cast<double>(timeThreaded) * 100.0 - 100.0;
        std::cout << "Time gain for one computation: " << gain << " % " << std::endl;
    }
}


/**
 * This class implements a tester for the multiplier that can launch a certain
 * number of threads that will call multiply in parallel. It allows to validate
 * the fact that multiply is reentrant.
 */
template<class ThreadedMultiplierType>
class MultiplierThreadedTester
{

public:
    MultiplierThreadedTester(int nbThreadsForTester) : nbThreadsForTester(nbThreadsForTester) {}

    ~MultiplierThreadedTester() = default;

    /**
     * This function is called to start the test. It will launch
     * nbThreads threads to challenge the multiplier in terms
     * of correct way to handle its reentrantness
     */
    void test(int matrixSize, int nbThreads, int nbBlocksPerRow)
    {
        //! A single multiplier, used by all threads in parallel
        auto threadedMultiplier = std::make_unique<ThreadedMultiplierType>(nbThreads);

        std::vector<std::unique_ptr<PcoThread>> threadList;

        for (int i = 0; i < nbThreadsForTester; i++) {
            auto *insiderThread = new PcoThread(test_int<ThreadedMultiplierType>,
                                                  matrixSize,
                                                  nbBlocksPerRow,
                                                  threadedMultiplier.get());

            threadList.push_back(std::unique_ptr<PcoThread>(insiderThread));
        }

        for (int i = 0; i < nbThreadsForTester; i++) {
            threadList[i]->join();
        }
    }

protected:
    int nbThreadsForTester;
};



#endif // MULTIPLIERTHREADEDTESTER_H
