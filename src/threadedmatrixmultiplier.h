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


    // Matrices qu'on va utiliser
    const SquareMatrix<T>* A = nullptr;
    const SquareMatrix<T>* B = nullptr;
    SquareMatrix<T>* C = nullptr;

    // Infos sur le bloc à calculer
    int blockRow = 0;     // indice du bloc en ligne
    int blockCol = 0;     // indice du bloc en colonne
    int blockSize = 0;    // taille d’un bloc

    // Taille totale matrice
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
    int nbJobFinished{0};

    /// \brief Sends a job to the buffer
    /// \param Reference to a ComputeParameters object which holds the necessary parameters to execute a job
    ///
    void sendJob(ComputeParameters<T> params) {
        monitorIn();             // Début section critique
        jobs.push(params);       // Ajouter un job
        signal(hasJob);          // Réveiller un worker
        monitorOut();            // Fin de la section critique
    }

    ///
    /// \brief Requests a job to the buffer
    /// \param Reference to a ComputeParameters object which holds the necessary parameters to execute a job
    /// \return true if a job is available, false otherwise
    ///
    bool getJob(ComputeParameters<T>& parameters)
    {
        monitorIn();

        if (jobs.empty() && !stop) {
            wait(hasJob);
        }

        // Si on arrête et qu'il n'y a rien, le worker peut sortir du moniteur
        if (stop && jobs.empty()) {
            monitorOut();
            return false;
        }

        parameters = jobs.front();
        jobs.pop();

        monitorOut();
        return true;
    }

    // Le worker appelle quand il a fini un job
    void jobDone()
    {
        monitorIn();
        nbJobFinished = nbJobFinished + 1;
        if(jobs.empty()){
            signal(noJob);
        }
        monitorOut();
    }

    // Appelé au destructeur pour débloquer les workers
    void shutdown(int nbWorkers)
    {
        monitorIn();
        stop = true;
        for (int i = 0; i < nbWorkers; ++i) {
            // On réveille ceux qui attendent
            signal(hasJob);

        }
        monitorOut();
    }

    // Permet au thread principal d'attendre sur les worker
    void waitOnJobs(int nbJob){
        monitorIn();

        while(nbJobFinished < nbJob){
            wait(noJob);
        }

        monitorOut();
    }

private:
    std::queue<ComputeParameters<T>> jobs;
    PcoHoareMonitor::Condition hasJob;
    bool stop = false;

    PcoHoareMonitor::Condition noJob;

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
        // Création des threads workers
        // Chaque thread va exécuter workerLoop
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
        // On arrête proprement les threads
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
        int n = A.size();
        int blockSize = n / nbBlocksPerRow;
        int totalJobs = nbBlocksPerRow * nbBlocksPerRow;

        // Réinitialisation du compteur
        buffer.nbJobFinished = 0;

        // Création des jobs je me dis que un job = un bloc
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

        // Attente que tous les jobs soient finis
        buffer.waitOnJobs(totalJobs);
    }

protected:
    int nbThreads;
    int nbBlocksPerRow;


private:

    Buffer<T> buffer;
    std::vector<PcoThread*> workers;


    static void workerLoop(void* arg)
    {
        ThreadedMatrixMultiplier<T>* self =
            static_cast<ThreadedMatrixMultiplier<T>*>(arg);

        ComputeParameters<T> p;

        // Le thread prend des jobs tant qu'il y en a
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
};




#endif // THREADEDMATRIXMULTIPLIER_H
