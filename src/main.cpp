#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#include "ArgParser.h"
#include "Logger.h"
#include "ParallelBubbleSort.h"
#include "RandomGenerator.h"
#include "Timer.h"

void generateArray(std::vector<int>& arr, size_t arrSize);

MPI_Comm spawn(char* cmd, int delayMs, int count);

void parentHelp() {
  printf("main ARRAY_SIZE TASK_COUNT [OPTION]\n");
  printf("\t ARRAY_SIZE - size of the array to sort (positive integer)\n");
  printf(
      "\t --delay COMPARE_DELAY_MS - compare opration delay in milliseconds "
      "used to simulate computation heavy operation (positive integer)\n");
  printf(
      "\t -v VERBOSE_LEVEL - 0 -> basic info, 1 -> debug print every pass "
      "(positive integer)\n");
}

void childHelp() {
  printf("main COMPARE_DELAY_MS\n");
  printf(
      "\t COMPARE_DELAY_MS - compare opration delay in milliseconds used to "
      "simulate computation heavy operation (positive integer)\n");
}

int main(int argc, char** argv) {
  /********** Initialize MPI **********/

  MPI_Init(&argc, &argv);
  int processCount;
  int commRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &commRank);
  MPI_Comm_size(MPI_COMM_WORLD, &processCount);

ArgParser parser{};
parser.addArgument("--delay", ArgParser::Type::integer);
parser.addArgument("-v", ArgParser::Type::integer);
parser.addArgument("--log", ArgParser::Type::flag);
parser.parse(argc - 2, argv + 2);

  if (commRank == 0) {
    try {
      if (argc < 2) {
        printf("Too few arguments!\n");
        throw std::invalid_argument{"Too few arguments"};
      }
      auto arrSize = std::stoi(argv[1]);
      auto compareDelay = 1;
      auto verboseLevel = 0;
      if (auto arg = parser.get("--delay")) {
        compareDelay = arg.value;
      }
      if (auto arg = parser.get("-v")) {
        verboseLevel = arg.value;
      }
      if (arrSize <= 0) {
        throw std::invalid_argument{"Size has to be greater than 0"};
      }
      if (processCount <= 1) {
        throw std::invalid_argument{"At least two processes are needed for sorting to work!"};
      }
      if (compareDelay < 0) {
        throw std::invalid_argument{"Delay cannot be negative!"};
      }
	  Logger logger{"Log.txt", "ArraySize", "TaskCount", "compareTimeMs", "comparisonCount", "actualTimeMs"};
      std::vector<int> arr;
      generateArray(arr, arrSize);
      Timer timer;
      timer.start();
      timer.start();
      auto workersCount = processCount - 1;
      ParallelBubbleSort bubbleSort{MPI_COMM_WORLD, static_cast<size_t>(workersCount),
                                    verboseLevel};
      auto comparisonCount = bubbleSort.sort(arr);

      printf("\n");
      printf("Result array: ");
      for (auto& e : arr) {
        printf("%d, ", e);
      }
      printf("\n");
      printf("Performed %ld comparisons.\n", comparisonCount);
      int approxSingleProcessTime = compareDelay * comparisonCount;
      int approxMultiProcessTime = approxSingleProcessTime / workersCount;
      printf("Approx single process time:\t%d ms \n", approxSingleProcessTime);
      printf("Approx multiprocess time:\t%d ms \n", approxMultiProcessTime);
      int actualTime = timer.elapsedTime<std::chrono::milliseconds>().count();
      printf("Sorting time:\t%d ms.\n", actualTime);
      printf("Multiprocess overhead time:\t%d ms.\n",
             actualTime - approxMultiProcessTime);
      if (parser.get("--log"))
      {
	     logger.log(arrSize, workersCount, compareDelay, comparisonCount, actualTime);
      }
      MPI_Abort(MPI_COMM_WORLD, 1);

    } catch (std::invalid_argument e) {
      printf("Invalid arguments! %s\n", e.what());
      parentHelp();
      MPI_Abort(MPI_COMM_WORLD, 1);
      return 1;
    }
  } else {
    int worldRank;
    int worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    auto delayMs = 1;
    if (auto arg = parser.get("--delay"))
    {
	    delayMs = arg.value;
    }
    ParallelBubbleSort::childProcess(MPI_COMM_WORLD, worldRank,
                                     std::chrono::milliseconds{delayMs});
  }

  /********** Finalize MPI **********/
  MPI_Finalize();
  return 0;
}

void generateArray(std::vector<int>& arr, size_t arrSize) {
  arr.reserve(arrSize);
  printf("This is the unsorted array: ");
  {
    RandomGenerator rg{-0xFFFF, 0xFFFF};
    for (auto i = 0U; i < arrSize; ++i) {
      auto genValue = rg.generate();
      arr.push_back(genValue);
      printf("%d ", genValue);
    }
  }
}
