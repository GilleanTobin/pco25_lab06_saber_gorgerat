#include <gtest/gtest.h>
#include <pcosynchro/pcotest.h>

#include "multipliertester.h"
#include "multiplierthreadedtester.h"
#include "threadedmatrixmultiplier.h"

#define ThreadedMultiplierType ThreadedMatrixMultiplier<int>

// Decommenting the next line allows to check for interlocking
// #define CHECK_DURATION

TEST(Multiplier, SingleThread){

#ifdef CHECK_DURATION
        ASSERT_DURATION_LE(30, ({
#endif // CHECK_DURATION
                               constexpr int MATRIXSIZE = 500;
                               constexpr int NBTHREADS = 1;
                               constexpr int NBBLOCKSPERROW = 5;

                               MultiplierTester<ThreadedMultiplierType> tester;

                               tester.test(MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                           }))
#endif // CHECK_DURATION

}


TEST(Multiplier, Simple){

#ifdef CHECK_DURATION
        ASSERT_DURATION_LE(30, ({
#endif // CHECK_DURATION
                               constexpr int MATRIXSIZE = 500;
                               constexpr int NBTHREADS = 4;
                               constexpr int NBBLOCKSPERROW = 5;

                               MultiplierTester<ThreadedMultiplierType> tester;

                               tester.test(MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                           }))
#endif // CHECK_DURATION

}


TEST(Multiplier, Reentering)
{

#ifdef CHECK_DURATION
    ASSERT_DURATION_LE(30, ({
#endif // CHECK_DURATION
                           constexpr int MATRIXSIZE = 500;
                           constexpr int NBTHREADS = 4;
                           constexpr int NBBLOCKSPERROW = 5;

                           MultiplierThreadedTester<ThreadedMultiplierType> tester(2);

                           tester.test(MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                       }))
#endif // CHECK_DURATION
}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
