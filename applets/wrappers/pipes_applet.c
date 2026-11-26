#include "exec_embedded.h"
#include "pipes_embedded.h"

int pipes_main(int argc, char **argv) {
    return run_embedded_bash("pipes.sh", pipes_sh_data, pipes_sh_data_len, argc, argv);
}
