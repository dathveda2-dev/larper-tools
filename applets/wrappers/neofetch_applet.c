#include "exec_embedded.h"
#include "neofetch_embedded.h"

int neofetch_main(int argc, char **argv) {
    return run_embedded_bash("neofetch", neofetch_data, neofetch_data_len, argc, argv);
}
