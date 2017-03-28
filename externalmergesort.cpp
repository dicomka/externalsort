#include "externalmergesort.h"

#include <algorithm>
#include <future>
#include <deque>
#include <map>
//#include <chrono>

bool read(std::ifstream& file, uint32_t& val)
{
    return file.read(reinterpret_cast<char*>(&val), sizeof(val)) ? true : false;
}

void read(std::ifstream& file, std::vector<uint32_t>& mas)
{
    if (!file.read((char*) mas.data(), sizeof(uint32_t) * mas.size())) {
        // Если прочитали меньше чем mas.size() элементов
        mas.resize(file.gcount() / sizeof(uint32_t));
    }
}

void write(std::ofstream& file, uint32_t& val)
{
    file.write(reinterpret_cast<char *>(&val), sizeof(val));
}

void write(std::ofstream& file, std::vector<uint32_t>& mas)
{
    file.write((char*)mas.data(), sizeof(uint32_t) * mas.size());
}

ExternalMergeSort::ExternalMergeSort(size_t size_buf, size_t max_pool)
    : max_size_buf_(size_buf),
      thread_pool_(max_pool)
{

}

bool ExternalMergeSort::run(const std::string& file_name)
{
    // Открывает файл для чтения в бинарном виде
    std::ifstream file_input(file_name, std::ios::binary | std::ios::in);
    if(!file_input.is_open()) {
        return false;
    }
    file_name_sort_.clear();
    files_name_.clear();

    // Разбивает основной файл на отсортированные куски длиной max_size_buf_ и записывает каждый в отдельный файл
    auto splt = thread_pool_.enqueue([&] {
        split(file_input);
        file_input.close();
    });
    // Если есть отсортированные файлы, объединяем их в один отсортированный
    while(file_input.is_open()) {
        mergeAll();
    }
    mergeAll();
    return true;
}

std::string ExternalMergeSort::getNameRezult()
{
    return file_name_sort_;
}

bool ExternalMergeSort::saveToFile(std::vector<uint32_t>& mas, const std::string& file_name) const
{
    std::ofstream file(file_name, std::ios::binary | std::ios::out);
    if (!file.is_open()) {
        return false;
    }
    write(file, mas);
    file.close();
    return true;
}

size_t ExternalMergeSort::split(std::ifstream& file)
{
    size_t count_files = 0;
    std::vector<uint32_t> data(max_size_buf_ * 0.7);
    while (!file.eof()) {
        // Считывает элементы блоками длины max_size_buf_ из файла
        read(file, data);
        std::sort(data.begin(), data.end());
        saveToFile(data, std::to_string(count_files));
        std::lock_guard<std::mutex> lgd(mtx_);
        files_name_.emplace_back(std::to_string(count_files++));
    }
    return count_files;
}

void ExternalMergeSort::mergeAll()
{
    // пока не останется один файл
    while (files_name_.size() > 1) {
        std::vector< std::future<std::string> > results;
        while (files_name_.size() > 1) {
            std::string name_file1;
            std::string name_file2;
            {
                std::lock_guard<std::mutex> lgd(mtx_);
                name_file1 = files_name_.front();
                files_name_.pop_front();
                name_file2 = files_name_.front();
                files_name_.pop_front();
            }
            // запускает объединение в отдельном потоке. Кол-во потоков опред. в конструкторе класса
            results.emplace_back(
                thread_pool_.enqueue([=] {
                    mergeFileBlock(name_file1, name_file2, name_file1 + name_file2);
                    remove(name_file1.c_str());
                    remove(name_file2.c_str());
                    return name_file1 + name_file2;
                })
            );
        }
        //
        for(auto && result: results) {
            std::string file_name = result.get();
            // имя отcортированного файла
            file_name_sort_ = file_name;
            std::lock_guard<std::mutex> lgd(mtx_);
            files_name_.emplace_back(file_name);
        }
    }
}

void ExternalMergeSort::mergeFile(const std::string& file_name_in1,
                              const std::string& file_name_in2,
                              const std::string& file_name_out)
{
    std::ifstream file_in1(file_name_in1);
    if(!file_in1.is_open()) {
        return;
    }
    std::ifstream file_in2(file_name_in2);
    if(!file_in2.is_open()) {
        return;
    }
    std::ofstream file_out(file_name_out);
    if(!file_out.is_open()) {
        return;
    }

    uint32_t val1, val2;
    read(file_in1, val1);
    read(file_in2, val2);
    while (!file_in1.eof() && !file_in2.eof()) {
        if (val1 < val2) {
            write(file_out, val1);
            read(file_in1, val1);
        } else {
            write(file_out, val2);
            read(file_in2, val2);
        }
    }
    // Считываем оставшиеся элементы из файла file_in1
    while (!file_in1.eof()) {
        write(file_out, val1);
        read(file_in1, val1);
    }
    // Считываем оставшиеся элементы из файла file_in2
    while (!file_in2.eof()) {
        write(file_out, val2);
        read(file_in2, val2);
    }

    file_in1.close();
    file_in2.close();
    file_out.close();
}

void ExternalMergeSort::mergeFileBlock(const std::string& file_name_in1,
                              const std::string& file_name_in2,
                              const std::string& file_name_out)
{
    std::ifstream file_in1(file_name_in1);
    if(!file_in1.is_open()) {
        return;
    }
    std::ifstream file_in2(file_name_in2);
    if(!file_in2.is_open()) {
        return;
    }
    std::ofstream file_out(file_name_out);
    if(!file_out.is_open()) {
        return;
    }

    const size_t max_size = max_size_buf_ * 0.15;
    // Два буфера под каждый из файлов чтобы считывать данные блоками
    std::vector<uint32_t> data1(max_size);
    std::vector<uint32_t> data2(max_size);
    size_t i = 0, j = 0;
    read(file_in1, data1);
    read(file_in2, data2);
    while (!data1.empty() && !data2.empty()) {
        if (i == data1.size()) { i = 0; read(file_in1, data1); }
        if (j == data2.size()) { j = 0; read(file_in2, data2); }
        while (i < data1.size() && j < data2.size()) {
            if (data1[i] < data2[j]) {
                write(file_out, data1[i++]);
            } else {
                write(file_out, data2[j++]);
            }
        }
    }
    // После основного цикла в массиве могли остаться элементы
    for (; i != data1.size(); ++i) {
        write(file_out, data1[i]);
    }
    // После основного цикла в массиве могли остаться элементы
    for (; j != data2.size(); ++j) {
        write(file_out, data2[j]);
    }
    // Считываем оставшиеся элементы из файла file_in1
    while (!file_in1.eof()) {
        // Считывает и пишет элементы блоками длины data1.size() из файла
        read(file_in1, data1);
        write(file_out, data1);
    }
    // Считываем оставшиеся элементы из файла file_in2
    while (!file_in2.eof()) {
        // Считывает и пишет элементы блоками длины data2.size() из файла
        read(file_in2, data2);
        write(file_out, data2);
    }

    file_in1.close();
    file_in2.close();
    file_out.close();
}
