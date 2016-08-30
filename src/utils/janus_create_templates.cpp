#include <stdlib.h>
#include <string.h>

#include "iarpa_janus.h"
#include "iarpa_janus_io.h"

const char *get_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

void printUsage()
{
    printf("Usage: janus_create_templates sdk_path temp_path data_path metadata_file templates_dir template_list_file template_role [-gpuindex <index>] [-algorithm <algorithm>] [-verbose]\n");
}

int main(int argc, char *argv[])
{
    int requiredArgs = 8;
    if ((argc < requiredArgs) || (argc > 14)) {
        printUsage();
        return 1;
    }

    const char *ext1 = get_ext(argv[4]);
    if (strcmp(ext1, "csv") != 0) {
        printf("metadata_file must be \".csv\" format.\n");
        return 1;
    } 

    // char *algorithm = "";
    std::string algorithm;
    bool verbose = false;
    int gpu_index = 0;

    for (int i = 0; i < argc - requiredArgs; i++) {
        if (strcmp(argv[requiredArgs+i],"-algorithm") == 0)
            algorithm = argv[requiredArgs+(++i)];
        else if (strcmp(argv[requiredArgs+i],"-gpuindex") == 0)
            gpu_index = atoi(argv[requiredArgs+(++i)]);
        else if (strcmp(argv[requiredArgs+i],"-verbose") == 0)
            verbose = true;
        else {
            fprintf(stderr, "Unrecognized flag: %s\n", argv[requiredArgs+i]);
            return 1;
        }
    }

    JANUS_ASSERT(janus_initialize(argv[1], argv[2], algorithm, gpu_index))
    JANUS_ASSERT(janus_create_templates_helper(argv[3], argv[4], argv[5], argv[6], static_cast<janus_template_role>(atoi(argv[7])), verbose))
    JANUS_ASSERT(janus_finalize())

    return EXIT_SUCCESS;
}
