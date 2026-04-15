#include "interfaces/cli/command_line.h"

#include <c10/util/Exception.h>

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        return nmc::interfaces::cli::run_cli(argc, argv);
    } catch (const c10::Error& error) {
        std::cerr << "LibTorch error: " << error.what() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
    }

    return 1;
}
