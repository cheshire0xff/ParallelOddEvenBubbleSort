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
  printf("\t TASK_COUNT - number of created tasks (positive integer)\n");
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
  MPI_Comm parent;
  MPI_Comm_get_parent(&parent);
  Logger logger{"Log.txt", "ArraySize", "TaskCount", "compareTimeMs", "comparisonCount", "actualTimeMs"};

  if (parent == MPI_COMM_NULL) {
    try {
      if (argc < 3) {
        printf("Too few arguments!\n");
        throw std::invalid_argument{"Too few arguments"};
      }
      auto arrSize = std::stoi(argv[1]);
      auto taskCount = std::stoi(argv[2]);

      ArgParser parser{};
      parser.addArgument("--delay", ArgParser::Type::integer);
      parser.addArgument("-v", ArgParser::Type::integer);
      parser.addArgument("--log", ArgParser::Type::flag);
      parser.parse(argc - 3, argv + 3);
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
      if (taskCount <= 0) {
        throw std::invalid_argument{"Task count has to be greater than 0"};
      }
      if (compareDelay < 0) {
        throw std::invalid_argument{"Delay cannot be negative!"};
      }
      std::vector<int> arr;
      generateArray(arr, arrSize);
      Timer timer;
      timer.start();
      auto childComm = spawn(argv[0], compareDelay, taskCount);
      printf("Spawning took %lims.\n",
             timer.elapsedTime<std::chrono::milliseconds>().count());
      timer.start();
      ParallelBubbleSort bubbleSort{childComm, static_cast<size_t>(taskCount),
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
      int approxMultiProcessTime = approxSingleProcessTime / taskCount;
      printf("Approx single process time:\t%d ms \n", approxSingleProcessTime);
      printf("Approx multiprocess time:\t%d ms \n", approxMultiProcessTime);
      int actualTime = timer.elapsedTime<std::chrono::milliseconds>().count();
      printf("Sorting time:\t%d ms.\n", actualTime);
      printf("Multiprocess overhead time:\t%d ms.\n",
             actualTime - approxMultiProcessTime);
      if (parser.get("--log"))
      {
	     logger.log(arrSize, taskCount, compareDelay, comparisonCount, actualTime);
      }
      MPI_Abort(childComm, 1);

    } catch (std::invalid_argument e) {
      printf("Invalid arguments! %s\n", e.what());
      parentHelp();
      return 1;
    }
  } else {
    int worldRank;
    int worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    int delayMs = 0;
    try {
      delayMs = std::stoi(argv[1]);
    } catch (std::invalid_argument e) {
      printf("Invalid arguments!\n");
      childHelp();
      return 5;
    }
    ParallelBubbleSort::childProcess(parent, worldRank,
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

MPI_Comm spawn(char* cmd, int delayMs, int count) {
  auto str = std::to_string(delayMs);
  char* data[2] = {str.data()};
  printf("Spawning: %d processes... ", count);
  MPI_Comm child;
  auto spawnErrors = new int[count];
  MPI_Comm_spawn(cmd, data, count, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &child,
                 spawnErrors);
  printf("success.\n");
  return child;
}
