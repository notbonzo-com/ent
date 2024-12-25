#include <preprocessor.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.ent>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];

    printf("Processing file: %s\n", filename);

    struct preprocessor pp = preprocessor_create(filename);

    if (pp.preprocessed_file) {
        printf("\n--- Preprocessed File ---\n%s\n", pp.preprocessed_file);
    } else {
        printf("\n--- No output generated. Check for errors. ---\n");
    }

    if (pp.include_count > 0) {
        printf("\n--- Included Files ---\n");
        for (size_t i = 0; i < pp.include_count; i++) {
            printf("%s\n", pp.includes[i]);
        }
    } else {
        printf("\n--- No includes found. ---\n");
    }

    if (pp.define_count > 0) {
        printf("\n--- Defined Constants ---\n");
        for (size_t i = 0; i < pp.define_count; i++) {
            printf("%s = %s\n", pp.defines[i].name, pp.defines[i].value);
        }
    } else {
        printf("\n--- No defines found. ---\n");
    }

    preprocessor_destroy(&pp);

    printf("\nProcessing complete.\n");
    return EXIT_SUCCESS;
}
