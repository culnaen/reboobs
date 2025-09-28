#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define DELIMITER '\''
#define MENU_CHR 'm'
#define START_POSITION 11
#define GRUB_CFG_PATH "/boot/grub/grub.cfg"
#define GRUBENV_PATH "/boot/grub/grubenv"
#define NEXT_ENTRY_KEY "next_entry="
#define NEXT_ENTRY_KEY_SIZE (sizeof(NEXT_ENTRY_KEY) - 1)

char **get_menuentries(char *path) {
  FILE *fp;
  char buf[256];
  size_t buf_size = 256, n, capacity;
  char *start_ptr;
  char *end_ptr;
  char *result;
  char **items;

  n = capacity = 0;
  items = NULL;

  fp = fopen(path, "r");
  if (fp == NULL) {
    perror("error");
    return 0;
  }

  while (fgets(buf, buf_size, fp)) {
    if (buf[0] == MENU_CHR) {
      start_ptr = buf + START_POSITION;
      if (start_ptr) {
        end_ptr = strchr(start_ptr, DELIMITER);
        if (end_ptr) {
          size_t len_string = end_ptr - start_ptr;
          result = (char *)malloc(len_string + 1);
          if (result == NULL) {
            perror("# error malloc");
            return 0;
          }
          strncpy(result, start_ptr, len_string);
          if (n >= capacity) {
            capacity = capacity == 0 ? 4 : capacity * 2;
            items = realloc(items, capacity * sizeof(char *));
            if (items == NULL) {
              perror("# realloc error");
              return 0;
            }
          }
          items[n] = result;
          printf("%zu. %s\n", n, items[n]);
          n++;
        }
      }
    }
  }

  fclose(fp);
  return items;
}

int set_nextentry(char *menuentry) {
  pid_t pid;

  pid = fork();
  if (pid == 0) {
    execl("/usr/bin/sudo", "sudo", "/usr/sbin/grub-reboot", menuentry,
          (char *)NULL);
    return 1;
  } else if (pid > 0) {
    int wstatus;
    if (waitpid(pid, &wstatus, 0) < 0) {
      perror("# waitpid");
      return 0;
    }
    if (!WIFEXITED(wstatus)) {
      perror("# chld did not exit");
      return 0;
    }
    if (WEXITSTATUS(wstatus) != 0) {
      perror("# non-zero exit");
      return 0;
    }
  } else {
    perror("# fork");
    return 0;
  }
  return 1;
}

int main() {
  FILE *fp;
  char buf[256];
  size_t buf_size = 256;
  char **items;
  size_t user_menuentry_index;
  char *user_menuentry;
  char *next_entry_env;
  size_t next_entry_env_size;
  bool reboot_flag = false;

  items = get_menuentries(GRUB_CFG_PATH);

  printf("your choice:");
  if (scanf("%zu", &user_menuentry_index) == 1 &&
      user_menuentry_index <= sizeof(items)) {

    user_menuentry = items[user_menuentry_index];

    set_nextentry(user_menuentry);

    next_entry_env_size = NEXT_ENTRY_KEY_SIZE + strlen(user_menuentry);
    next_entry_env = (char *)malloc(next_entry_env_size);
    if (next_entry_env == NULL) {
      perror("# malloc next_entry_env");
      return 1;
    }
    strcpy(next_entry_env, NEXT_ENTRY_KEY);
    strcat(next_entry_env, user_menuentry);
    strcat(next_entry_env, "\0");
    fp = fopen(GRUBENV_PATH, "r");
    if (fp == NULL) {
      perror("# error read /boot/grub/grubenv");
      return 1;
    }
    while (fgets(buf, buf_size, fp)) {
      if (buf[0] == '#') {
        continue;
      }
      if (!strchr(buf, '=')) {
        continue;
      }

      buf[strcspn(buf, "\n")] = '\0';
      if (strcmp(buf, next_entry_env) == 0) {
        reboot_flag = true;
        break;
      }
    }
  }

  if (reboot_flag) {
    fclose(fp);
    free(items);
    // execl("/usr/bin/sudo", "sudo", "shutdown", "-r", "0", (char *)NULL);
  }
}
