#pragma once

#include <string>

enum ReceiveType {
    TYPE_NONE = 0,
    SEND = 1 << 0,
    LIST = 1 << 1,
    READ = 1 << 2,
    DEL = 1 << 3,
    LOGIN = 1 << 4,
    REPLY = 1 << 5
};