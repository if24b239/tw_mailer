#pragma once

#include <threadutils.h>
#include <fstream>
#include <string>
#include <filesystem>

enum class StorageMode {
    READ,
    WRITE,
    NONE
};

class FileAccessors {
private:
    std::ofstream outfile;
    std::ifstream infile;

public:
    std::string filename;
    
    FileAccessors(std::string file) : filename(file) {}

    void open_ofstream() {
        if (!outfile.is_open()) {
            outfile.open(filename, std::ios::app);
        }
    }

    void open_ifstream() {
        if (!infile.is_open()) {
            infile.open(filename, std::ios::app);
        }
    }

    void close() {
        if (infile.is_open()) {
            infile.close();
        }

        if (outfile.is_open()) {
            outfile.close();
        }
    }

    std::ofstream* get_ofstream() {
        return &outfile;
    }

    std::ifstream* get_ifstream() {
        return &infile;
    }
};

class Storage : public thread_obj<FileAccessors> {
public:
    Storage(const std::string& file) : thread_obj<FileAccessors>(FileAccessors{file}) {
        if (!std::filesystem::exists(file))
            auto acc = this->access(StorageMode::WRITE); // creates an empty file if it does not exist.
    }

    class access_proxy_file : public access_proxy {
    public:
        access_proxy_file(std::mutex& mtx, FileAccessors& obj, StorageMode mode) : access_proxy(mtx, obj) {
            if (mode == StorageMode::WRITE) {
                obj.open_ofstream();
            }

            if (mode == StorageMode::READ) {
                obj.open_ifstream();
            }
        }

        ~access_proxy_file() {
            (*this)->close();
        }
    };

    access_proxy_file access(StorageMode mode) {
        return access_proxy_file(obj_mutex, object, mode);
    }
};