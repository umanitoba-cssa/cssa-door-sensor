#pragma once

#include "WifiSvc.hpp"

typedef void (*print_fn)(const char *);
typedef int (*read_int_fn)(int);
typedef String (*read_str_fn)(bool);

class SetupService {
  private:
    print_fn print;
    read_int_fn intReader;
    read_str_fn strReader;

    void menu();
    void wifi();
    void webhook();
    void clear();

  public:
    SetupService();

    void start(print_fn print, read_int_fn intReader, read_str_fn strReader);
};

extern SetupService SetupSvc;