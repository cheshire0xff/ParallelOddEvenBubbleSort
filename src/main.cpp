#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#include "ArgParser.h"
#include "Logger.h"
#include "ParallelBubbleSort.h"
#include "RandomGenerator.h"
#include "Timer.h"

namespace defaults {
constexpr int kDelayMs = 1;
constexpr int kMin = -0xFFFF;
constexpr int kMax = 0xFFFF;
constexpr int kVerboseLevel = 0;
}  // namespace defaults

void parentHelp() {
    printf("main ARRAY_SIZE [OPTION]\n");
    printf("\tARRAY_SIZE\n");
    printf("\t\tsize of the array to sort (positive integer)\n");
    printf("\t--delay COMPARE_DELAY_MS\n");
    printf(
        "\t\tcompare opration delay in milliseconds "
        "used to simulate computation heavy operation (positive integer)\n");
    printf("\t-v VERBOSE_LEVEL\n");
    printf("\t\t0 = basic info, 1 = debug print every pass , 2 = detailed\n");
    printf("\t--log\n");
    printf("\t\tgenerate statistics Log file\n");
    printf("\t--min VALUE\n");
    printf("\t\tmin value of generated array (integer)\n");
    printf("\t--max VALUE\n");
    printf("\t\tmax value of generated array (integer)\n");
}
void generateArray(std::vector<int>& arr, size_t arrSize, int min, int max);

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
    parser.addArgument("--min", ArgParser::Type::integer);
    parser.addArgument("--max", ArgParser::Type::integer);
    parser.parse(argc - 2, argv + 2);

    if (commRank == 0) {
        try {
            if (argc < 2) {
                throw std::invalid_argument{"Too few arguments"};
            }
            auto arrSize = std::stoi(argv[1]);
            auto compareDelay = defaults::kDelayMs;
            auto verboseLevel = defaults::kVerboseLevel;
            auto min = defaults::kMin;
            auto max = defaults::kMax;
            if (auto arg = parser.get("--delay")) {
                compareDelay = arg.value;
            }
            if (auto arg = parser.get("-v")) {
                verboseLevel = arg.value;
            }
	    if (auto arg = parser.get("--min"))
	    {
		    min = arg.value;
	    }
	    if (auto arg = parser.get("--max"))
	    {
		    max = arg.value;
	    }
            if (arrSize <= 0) {
                throw std::invalid_argument{"Size has to be greater than 0"};
            }
            if (processCount <= 1) {
                throw std::invalid_argument{
                    "At least two processes are needed for sorting to work!"};
            }
            if (compareDelay < 0) {
                throw std::invalid_argument{"Delay cannot be negative!"};
            }
            if (min >= max) {
                throw std::invalid_argument{
                    "Min array value has to be smaller than max!"};
            }
            Logger logger{"Log.txt",       "ArraySize",       "TaskCount",
                          "compareTimeMs", "comparisonCount", "actualTimeMs"};
            std::vector<int> arr;
            generateArray(arr, arrSize, min, max);
            Timer timer;
            timer.start();
            auto workersCount = processCount - 1;
            ParallelBubbleSort bubbleSort{MPI_COMM_WORLD,
                                          static_cast<size_t>(workersCount),
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
            printf("Approx single process time:\t%d ms \n",
                   approxSingleProcessTime);
            printf("Approx multiprocess time:\t%d ms \n",
                   approxMultiProcessTime);
            int actualTime =
                timer.elapsedTime<std::chrono::milliseconds>().count();
            printf("Sorting time:\t%d ms.\n", actualTime);
            printf("Multiprocess overhead time:\t%d ms.\n",
                   actualTime - approxMultiProcessTime);
            if (parser.get("--log")) {
                logger.log(arrSize, workersCount, compareDelay, comparisonCount,
                           actualTime);
            }

            printf("Exiting...");
            ParallelBubbleSort::stopChildProcesses(MPI_COMM_WORLD);
            printf(" success.\n");
        } catch (std::invalid_argument e) {
            printf("Invalid arguments! %s\n", e.what());
            parentHelp();
            printf("Exiting...");
            ParallelBubbleSort::stopChildProcesses(MPI_COMM_WORLD);
            printf(" success.\n");
            return 1;
        }
    } else {
        int worldRank;
        int worldSize;
        MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
        MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
        auto delayMs = 1;
        if (auto arg = parser.get("--delay")) {
            delayMs = arg.value;
        }
        ParallelBubbleSort::childProcess(MPI_COMM_WORLD, worldRank,
                                         std::chrono::milliseconds{delayMs});
    }

    /********** Finalize MPI **********/
    MPI_Finalize();
    return 0;
}

void generateArray(std::vector<int>& arr, size_t arrSize, int min, int max) {
    arr.reserve(arrSize);
    printf("This is the unsorted array: ");
    {
        RandomGenerator rg{min, max};
        for (auto i = 0U; i < arrSize; ++i) {
            auto genValue = rg.generate();
            arr.push_back(genValue);
            printf("%d ", genValue);
        }
    }
}
