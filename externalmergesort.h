#ifndef EXTERNALMERGESORT_H
#define EXTERNALMERGESORT_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
//#include <atomic>

#include "ThreadPool.h"

// прочитать один элемент из файла
bool read(std::ifstream& , uint32_t& val);

// прочитать массив элементов из файла
void read(std::ifstream& , std::vector<uint32_t>&);

// записать один элемент в файл
void write(std::ofstream& , uint32_t&);

// запись массива элементов в файл
void write(std::ofstream& , std::vector<uint32_t>&);

// Внешняя сортировка слиянием
class ExternalMergeSort
{
public:
    ExternalMergeSort(size_t, size_t);

    // запускает сортировку
    bool run(const std::string& );

    // возвращает имя отсортированного файла
    std::string getNameRezult();

private:

    // сохраняет массив в файл. Если файл уже существовал, все предыдущие данные удаляются.
    bool saveToFile(std::vector<uint32_t>& , const std::string& ) const;

    // разбивает исходный файл на отсортированные файлы длиной max_size_buf_
    size_t split(std::ifstream& );

    // слить(соединить) все файлы
    void mergeAll();

    // соединить два файла в один
    void mergeFile(const std::string& in1, const std::string& in2, const std::string& out);
    // соединить два файла в один (считывает элементы из файла блоками)
    void mergeFileBlock(const std::string& in1, const std::string& in2, const std::string& out);

    std::mutex mtx_;
    std::deque<std::string> files_name_; // имена отсортированных файлов (для дальнейшего слияниях их в один файл)
    //std::atomic_size_t name_files_{0}; // имена временных файлов с отсортированными элементами

    size_t max_size_buf_{(4 * 1024 * 1024) / sizeof(uint32_t)}; // максимальная длина буфера использ. при сортировки
    std::string file_name_sort_;

    ThreadPool thread_pool_;
    std::vector< std::future<std::string> > results_;
};

#endif // EXTERNALMERGESORT_H
