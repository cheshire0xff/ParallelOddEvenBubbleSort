#ifndef INC_LOGGER_H__
#define INC_LOGGER_H__

#include <fstream>

template <typename... Columns>
class Logger {
   public:
    std::array<std::string, sizeof...(Columns)> arr;
    std::string filename;

   public:
    template <typename Indices = std::make_index_sequence<sizeof...(Columns)>>
    constexpr Logger(const std::string& filename, const Columns&... args)
        : filename(filename) {
        fill(Indices{}, args...);
    }

    template <typename... Values>
    void log(const Values&... values) {
        static_assert(sizeof...(Columns) == sizeof...(Values));
        auto file = std::fstream{filename, file.app};
        if (file.tellp() == 0) {
            for (auto& column : arr) {
                file << column << ",";
            }
            file << "\n";
        }
        ((file << values << ","), ...);
        file << "\n";
    }

   private:
    template <size_t... I>
    void fill(std::index_sequence<I...>, const Columns&... args) {
        ((arr[I] = args), ...);
    }
};

#endif // INC_LOGGER_H__
