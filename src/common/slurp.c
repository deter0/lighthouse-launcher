#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <errno.h>

char *slurp_file(const char *file_path) {
  FILE *file = fopen(file_path, "rb");
  if (!file) {
    fprintf(stderr, "ERROR: Opening file %s:%s.\n", file_path, strerror(errno));
    return NULL;
  }
  
  long file_size = 0;
  fseek(file, 0L, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0L, SEEK_SET);
  
  char *file_buffer = malloc(file_size + 1);
  file_buffer[file_size] = 0;
  
  if (fread(file_buffer, file_size, 1, file) != 1) {
    fprintf(stderr, "ERROR: Reading file %s:%s.\n", file_path, strerror(errno));
    free(file_buffer);
    fclose(file);
    return NULL;
  }
  
  fclose(file);
  
  return file_buffer;
}
