#ifndef MATRIX_H
#define MATRIX_H

#include <iostream>
#include <vector>

/**
 * A class representing a basic matrix.
 * It is a template so as to be generic enough.
 * The only requirement is that T should have a * operator in order to let
 * the multiplication be done correctly.
 * */
template<class T>
class Matrix
{
public:
    Matrix(int sx, int sy)
    {
        array = std::vector<T>(sx * sy);
        sizeX = sx;
        sizeY = sy;
    }

    virtual ~Matrix() = default;

    inline T element(int x, int y) const
    {
        return array[sizeX * y + x];
    }

    inline void setElement(int x, int y, T value)
    {
        array[sizeX * y + x] = value;
    }

    void print() const
    {
        for (int y = 0; y < sizeY; y++) {
            for (int x = 0; x < sizeX; x++) {
                std::cout << element(x, y) << " ";
            }
            std::cout << std::endl;
        }
    }

    [[nodiscard]] int getSizeX() const { return sizeX; }

    [[nodiscard]] int getSizeY() const { return sizeY; }

    /**
     * This function simply compares two matrices and display the first
     * unmatching element if there exist one.
     */
    void compare(Matrix<T>& other) const
    {
        for (int i = 0; i < getSizeX(); i++) {
            for (int j = 0; j < getSizeY(); j++) {
                if (this->element(i, j) != other.element(i, j)) {
                    std::cout << "Error in matrix calculation" << std::endl;
                    std::cout << "i= " << i << "j= " << j << "M1(i,j)= " << this->element(i, j)
                              << "M2(i,j)= " << other.element(i, j) << std::endl;
                    return;
                }
            }
        }
        std::cout << "No error in calculus" << std::endl;
    }

protected:
    std::vector<T> array;
    int sizeX;
    int sizeY;
};

/**
 * A square matrix is simply a matrix with the same size for
 * both rows and columns.
 */
template<class T>
class SquareMatrix : public Matrix<T>
{
public:
    SquareMatrix(int size) : Matrix<T>(size, size) {}

    int size() const
    {
        return this->sizeX;
    }
};


#endif // MATRIX_H
