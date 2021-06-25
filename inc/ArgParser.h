#ifndef INC_ARG_PARSER
#define INC_ARG_PARSER
#include <map>
#include <string>

class ArgParser {
   public:
    enum class Type { integer, flag };

    void addArgument(const std::string& arg, Type type) {
        arguments[arg] = type;
    }

    void parse(int argc, char** argv) {
        for (int i = 0; i < argc; ++i) {
            auto arg = argv[i];
            auto resultPtr = arguments.find(arg);
            if (resultPtr == arguments.end()) {
                throw std::invalid_argument{"Unrecognized argument!"};
            }
            auto type = resultPtr->second;
            switch (type) {
                case Type::integer:
                    if (i + 1 >= argc) {
                        throw std::invalid_argument{"Flag needs a value!"};
                    }
                    ++i;
                    results[resultPtr->first] = std::stoi(argv[i]);
                    break;
                case Type::flag:
                    results[resultPtr->first] = -1;
                    break;
            }
        }
    }
    struct Result {
        bool found;
        int value;
        operator bool() const { return found; }
    };
    Result get(const std::string& arg) {
        auto ptr = results.find(arg);
        if (ptr == results.end()) {
            return {false, -1};
        } else {
            return {true, ptr->second};
        }
    }

   private:
    std::map<const std::string, Type> arguments;
    std::map<const std::string, int> results;
};

#endif
