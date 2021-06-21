#ifndef INC_PARALLEL_BUBBLE_SORT
#define INC_PARALLEL_BUBBLE_SORT

#include <mpi.h>
#include <fstream>
#include <vector>
#include "Timer.h"

class ParallelBubbleSort {
 public:
  enum step { even, odd };
  enum tags { compareRequest, compareResponse };
  ParallelBubbleSort(MPI_Comm taskComm, size_t taskCount, int verbose = 0)
      : _taskComm(taskComm), _taskCount(taskCount), _verbose(verbose) {}
  size_t sort(std::vector<int>& arr) {
    step state = odd;
    size_t comparisonCount = 0;
    for (auto pass = 0U; pass < arr.size(); ++pass) {
      auto pairsToProcess = state == even ? arr.size() / 2 : arr.size() / 2 - 1;
      comparisonCount += pairsToProcess;  // just for statistics
      int startIndex = state;
      while (pairsToProcess) {
        auto chunkToProcess = pairsToProcess;
        if (pairsToProcess > _taskCount) {
          chunkToProcess = _taskCount;
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
        printf("Pass %d, %s: \n", pass, state == even ? "even" : "odd ");
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
  static void childProcess(MPI_Comm& parent,
                           int,
                           std::chrono::milliseconds delay) {
    while (true) {
      int buf[2];
      MPI_Recv(buf, 2, MPI_INT, 0, compareRequest, parent, MPI_STATUS_IGNORE);
      // printf("child %d, received: %d, %d\n", rank, buf[0], buf[1]);
      Timer t{};
      t.start();
      while (t.elapsedTime<std::chrono::milliseconds>().count() <
             delay.count()) {
      };
      if (buf[0] > buf[1]) {
        std::swap(buf[0], buf[1]);
      }
      MPI_Send(buf, 2, MPI_INT, 0, compareResponse, parent);
    }
  }

 private:
  void processPairs(std::vector<int>& arr, int startIndex, int count) {
    // gather compared pairs
    auto sendIndex = startIndex;
    for (auto i = 0; i < count; ++i) {
      MPI_Send(arr.data() + sendIndex, 2, MPI_INT, i, compareRequest,
               _taskComm);
      sendIndex += 2;
    }
    // gather compared pairs
    auto receiveIndex = startIndex;
    for (auto i = 0; i < count; ++i) {
      MPI_Recv(arr.data() + receiveIndex, 2, MPI_INT, i, compareResponse,
               _taskComm, MPI_STATUS_IGNORE);
      receiveIndex += 2;
    }
  }
  MPI_Comm _taskComm;
  size_t _taskCount;
  int _verbose;
};

#endif
