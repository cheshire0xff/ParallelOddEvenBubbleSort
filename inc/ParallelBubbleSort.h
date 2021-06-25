#ifndef INC_PARALLEL_BUBBLE_SORT_H__
#define INC_PARALLEL_BUBBLE_SORT_H__

#include <mpi.h>
#include <fstream>
#include <vector>
#include "Timer.h"

class ParallelBubbleSort {
   public:
    enum eStep { even, odd };
    enum eTags { request, response };
    enum eRequestType { compare, exit };

    ParallelBubbleSort(MPI_Comm taskComm, size_t taskCount, int verbose = 0)
        : _taskComm(taskComm), _taskCount(taskCount), _verbose(verbose) {}
    size_t sort(std::vector<int>& arr) {
        eStep state = odd;
        size_t comparisonCount = 0;
        for (auto pass = 0U; pass < arr.size(); ++pass) {
            size_t pairsToProcess =
                state == even ? arr.size() / 2 : arr.size() / 2 - 1;
            comparisonCount += pairsToProcess;  // just for statistics
            auto optimalTaskCount = getOptimalSplit(pairsToProcess);
            if (_verbose >= 2) {
                printf("Optimal taskSplit: %ld.\n", optimalTaskCount);
            }
            int startIndex = state;

            while (pairsToProcess) {
                auto chunkToProcess = pairsToProcess;
                if (pairsToProcess > optimalTaskCount) {
                    chunkToProcess = optimalTaskCount;
                }
                if (_verbose >= 2) {
                    printf("processing %ld pairs, starting from %d index.\n",
                           chunkToProcess, startIndex);
                }
                processPairs(arr, startIndex, chunkToProcess);
                startIndex += 2 * chunkToProcess;
                pairsToProcess -= chunkToProcess;
            }
            if (_verbose >= 1) {
                printf("Pass %d, %s: \n", pass,
                       state == even ? "even" : "odd ");
            }
            if (_verbose >= 2) {
                for (auto& e : arr) {
                    printf("%d, ", e);
                }
                printf("\n");
            }
            if (state == odd) {
                state = even;
            } else {
                state = odd;
            }
        }
        return comparisonCount;
    }
    static void childProcess(MPI_Comm parent,
                             int,
                             std::chrono::milliseconds delay) {
        while (true) {
            int buf[3];
            MPI_Recv(buf, 3, MPI_INT, 0, request, parent, MPI_STATUS_IGNORE);
            auto rt = static_cast<eRequestType>(buf[0]);
            switch (rt) {
                case compare: {
                    Timer t{};
                    t.start();
                    while (t.elapsedTime<std::chrono::milliseconds>().count() <
                           delay.count()) {
                    };
                    if (buf[1] > buf[2]) {
                        std::swap(buf[1], buf[2]);
                    }
                    MPI_Send(buf + 1, 2, MPI_INT, 0, response, parent);
                } break;
                default:
                    return;
            }
        }
    }
    static void stopChildProcesses(MPI_Comm comm) {
        int sendBuf[3]{exit, 0, 0};
        int processCount;
        MPI_Comm_size(comm, &processCount);
        for (auto i = 1; i < processCount; ++i) {
            MPI_Send(sendBuf, 3, MPI_INT, i, request, comm);
        }
    }

   private:
    void processPairs(std::vector<int>& arr, int startIndex, int count) {
        // gather compared pairs
        auto sendIndex = startIndex;
        int sendBuf[3]{request, 0, 0};
        for (auto i = 1; i <= count; ++i) {
            sendBuf[1] = arr.data()[sendIndex];
            sendBuf[2] = arr.data()[sendIndex + 1];
            MPI_Send(sendBuf, 3, MPI_INT, i, request, _taskComm);
            sendIndex += 2;
        }
        // gather compared pairs
        auto receiveIndex = startIndex;
        for (auto i = 1; i <= count; ++i) {
            MPI_Recv(arr.data() + receiveIndex, 2, MPI_INT, i, response,
                     _taskComm, MPI_STATUS_IGNORE);
            receiveIndex += 2;
        }
    }

    size_t getOptimalSplit(int pairsCount) const {
        for (auto i = 1; i <= pairsCount; ++i) {
            size_t tasks = pairsCount / i;
            if (pairsCount % i) {
                ++tasks;
            }
            if (tasks <= _taskCount) {
                return tasks;
            }
        }
        return 1;
    }

   private:
    MPI_Comm _taskComm;
    size_t _taskCount;
    int _verbose;
};

#endif // INC_PARALLEL_BUBBLE_SORT_H__
