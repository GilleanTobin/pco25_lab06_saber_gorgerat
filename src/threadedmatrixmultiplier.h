#ifndef THREADEDMATRIXMULTIPLIER_H
#define THREADEDMATRIXMULTIPLIER_H

#include <pcosynchro/pcoconditionvariable.h>
#include <pcosynchro/pcohoaremonitor.h>
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>
#include <queue> // pour stocker les jobs dans le buffer et pour avoir une structure FIFO pour modèle producteur-consommateur
#include <vector> //pour stocker les threads

#include "abstractmatrixmultiplier.h"
#include "matrix.h"

///
/// A class that holds the necessary parameters for a thread to do a job.
///
template<class T>
class ComputeParameters
{
public:

    /* Maybe some parameters */

    //matrices qu'on va utiliser, hier on a discuté et je t'ai montré des variables mais c'st mieux d'utiliser des pointeurs car ça évite que les matricess soient copiées...
    //les copier pour chaque thread coûterait beaucoup de temps et de mémoire.
    const SquareMatrix<T>* A = nullptr;
    const SquareMatrix<T>* B = nullptr;
    SquareMatrix<T>* C = nullptr;

    //infos sur le bloc à calculer
    int blockRow = 0;     // indice du bloc en ligne
    int blockCol = 0;     // indice du bloc en colonne
    int blockSize = 0;    // taille d’un bloc

    //taille totale matrice
    int matrixSize = 0;
};


/// As a suggestion, a buffer class that could be used to communicate between
/// the workers and the main thread...
///
/// Here we only wrote two potential methods, but there could be more at the end...
///
template<class T>
class Buffer: public PcoHoareMonitor
{
public:
    int nbJobFinished{0}; // Keep this updated
    /* Maybe some parameters */

    ///
    /// \brief Sends a job to the buffer
    /// \param Reference to a ComputeParameters object which holds the necessary parameters to execute a job
    ///
    void sendJob(ComputeParameters<T> params) {
        monitorIn();              //début section critique
        jobs.push(params);        //ajouter un job
        signal(hasJob);          //réveiller un worker
        monitorOut();             //fin de la section critique
    }

    ///
    /// \brief Requests a job to the buffer
    /// \param Reference to a ComputeParameters object which holds the necessary parameters to execute a job
    /// \return true if a job is available, false otherwise
    ///
    bool getJob(ComputeParameters<T>& parameters)
    {
        monitorIn();

        while (jobs.empty() && !stop) {
            wait(hasJob);
        }

        //si on arrête et qu'il n'y a rien, le worker peut sortir
        if (stop && jobs.empty()) {
            monitorOut();
            return false;
        }

        parameters = jobs.front();
        jobs.pop();

        monitorOut();
        return true;
    }

    //le worker appelle ça quand il a fini un job
    void jobDone()
    {
        monitorIn();
        nbJobFinished = nbJobFinished + 1;
        monitorOut();
    }

    //appelé au destructeur pour débloquer les workers
    void shutdown(int nbWorkers)
    {
        monitorIn();
        stop = true;
        for (int i = 0; i < nbWorkers; ++i) {
            signal(hasJob); //on réveille ceux qui attendent
        }
        monitorOut();
    }

private:
    std::queue<ComputeParameters<T>> jobs;
    PcoHoareMonitor::Condition hasJob;
    bool stop = false;

    /* Maybe more methods */
};


///
/// A multi-threaded multiplicator. multiply() should at least be reentrant.
/// It is up to you to offer a very good parallelism.
///
template<class T>
class ThreadedMatrixMultiplier : public AbstractMatrixMultiplier<T>
{

public:
    ///
    /// \brief ThreadedMatrixMultiplier
    /// \param nbThreads Number of threads to start
    /// \param nbBlocksPerRow Default number of blocks per row, for compatibility with SimpleMatrixMultiplier
    ///
    /// The threads shall be started from the constructor
    ///
    ThreadedMatrixMultiplier(int nbThreads, int nbBlocksPerRow = 0)
        : nbThreads(nbThreads), nbBlocksPerRow(nbBlocksPerRow)
    {
        // TODO
        //création des threads workers
        //chaque thread va exécuter workerLoop
        for (int i = 0; i < nbThreads; i++) {
            workers.push_back(new PcoThread(workerLoop, this));
        }
    }

    ///
    /// In this destructor we should ask for the termination of the computations. They could be aborted without
    /// ending into completion.
    /// All threads have to be
    ///
    ~ThreadedMatrixMultiplier()
    {
        // TODO
        //on arrête proprement les threads, on apprends des erreurs des labos précédents mashallah
        buffer.shutdown(nbThreads);

        for (int i = 0; i < (int)workers.size(); i++) {
            workers[i]->join();
            delete workers[i];
        }
    }
    ///
    /// \brief multiply
    /// \param A First matrix
    /// \param B Second matrix
    /// \param C Result of AxB
    ///
    /// For compatibility reason with SimpleMatrixMultiplier
    void multiply(const SquareMatrix<T>& A, const SquareMatrix<T>& B, SquareMatrix<T>& C) override
    {
        multiply(A, B, C, nbBlocksPerRow);
    }

    ///
    /// \brief multiply
    /// \param A First matrix
    /// \param B Second matrix
    /// \param C Result of AxB
    /// \param nbBlocksPerRow Number of blocks per row (or columns)
    ///
    /// Executes the multithreaded computation, by decomposing the matrices into blocks.
    /// nbBlocksPerRow must divide the size of the matrix.
    ///
    void multiply(const SquareMatrix<T>& A, const SquareMatrix<T>& B, SquareMatrix<T>& C, int nbBlocksPerRow)
    {
        // OK, computation is done correctly, but... Is it really multithreaded?!?
        // TODO : Get rid of the next lines and do something meaningful
        int n = A.size();
        int blockSize = n / nbBlocksPerRow;
        int totalJobs = nbBlocksPerRow * nbBlocksPerRow;

        //réinitialisation du compteur
        buffer.nbJobFinished = 0;

        //création des jobs je me dis que un job = un bloc
        for (int br = 0; br < nbBlocksPerRow; br++) {
            for (int bc = 0; bc < nbBlocksPerRow; bc++) {
                ComputeParameters<T> p;
                p.A = &A;
                p.B = &B;
                p.C = &C;
                p.blockRow = br;
                p.blockCol = bc;
                p.blockSize = blockSize;
                p.matrixSize = n;

                buffer.sendJob(p);
            }
        }

        //attente que tous les jobs soient finis
        while (buffer.nbJobFinished < totalJobs) {

        }
    }

protected:
    int nbThreads;
    int nbBlocksPerRow;


private:
    static void workerLoop(void* arg)
    {
        ThreadedMatrixMultiplier<T>* self =
            static_cast<ThreadedMatrixMultiplier<T>*>(arg);

        ComputeParameters<T> p;

        //le thread prend des jobs tant qu'il y en a
        while (self->buffer.getJob(p)) {
            self->computeBlock(p);
            self->buffer.jobDone();
        }
    }

    void computeBlock(const ComputeParameters<T>& p)
    {
        int rowStart = p.blockRow * p.blockSize;
        int colStart = p.blockCol * p.blockSize;

        for (int i = rowStart; i < rowStart + p.blockSize; i++) {
            for (int j = colStart; j < colStart + p.blockSize; j++) {
                T result = 0;
                for (int k = 0; k < p.matrixSize; k++) {
                    result += p.A->element(k, j) * p.B->element(i, k);
                }
                p.C->setElement(i, j, result);
            }
        }
    }

private:
    Buffer<T> buffer;
    std::vector<PcoThread*> workers;
};




#endif // THREADEDMATRIXMULTIPLIER_H
