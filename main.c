#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define DELIMITER '\''
#define MENU_CHR 'm'
#define START_DELIMITER_POSITION 10

int main() {
  FILE *fp;
  char buf[256];
  size_t buf_size = 256;
  char *start_ptr;
  char *end_ptr;
  char *result;
  char **items;
  size_t n;
  size_t capacity;
  size_t choice;
  pid_t pid;

  n = 0;
  capacity = 0;
  items = NULL;

  fp = fopen("/boot/grub/grub.cfg", "r");
  if (fp == NULL) {
    perror("error");
  }

  while (fgets(buf, buf_size, fp)) {
    if (buf[0] == MENU_CHR) {
      start_ptr = buf + START_DELIMITER_POSITION;
      if (start_ptr) {
        end_ptr = strchr(start_ptr + 1, DELIMITER);
        if (end_ptr) {
          size_t len_string = end_ptr - start_ptr;
          result = (char *)malloc(len_string);
          if (result == NULL) {
            return 0;
          }
          strncpy(result, start_ptr, len_string + 1);
          printf("%zu. %s\n", n, result);
          if (n >= capacity) {
            capacity = capacity == 0 ? 4 : capacity * 2;
            items = realloc(items, capacity * sizeof(char *));
            if (!items) {
              perror("realloc");
              exit(-1);
            }
          }
          items[n] = result;
          free(result);
          n++;
        }
      }
    }
  }

  fclose(fp);

  printf("your choice:");
  if (scanf("%zu", &choice) == 1 && choice <= n) {
    pid = fork();
    if (pid == 0) {
      execl("/usr/bin/sudo", "sudo", "/usr/sbin/grub-reboot", items[choice],
            (char *)NULL);
      return 1;
    } else if (pid > 0) {
      waitpid(pid, NULL, 0);
    }

    free(items);

    pid = fork();
    if (pid == 0) {
      execl("/usr/bin/sudo", "sudo", "shutdown", "-r", "0", (char *)NULL);
      return 1;
    } else if (pid > 0) {
      waitpid(pid, NULL, 0);
    }
  }
}
