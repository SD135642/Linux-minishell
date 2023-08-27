#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define MAX_COMMAND_LEN 4096
#define MAX_TOKENS 2048

#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

volatile sig_atomic_t signal_val = 0;

void catch_signal(int sig) {
  signal_val = sig;
}

void signal_check() {
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = catch_signal;
  action.sa_flags = SA_SIGINFO;

  if (sigaction(SIGINT, &action, NULL) == -1) {
    perror("sigaction(SIGINT)\n");
    exit(EXIT_FAILURE);
  }
}

void arg_check(int argc) {
  if (argc > 1) {
    fprintf(stderr, "Error: Too many commandline arguments received.\n");
    exit(EXIT_FAILURE);
  }
}

void free_arrays(char *cwd, char *input, char **tokens) {
  free(cwd);
  if (input != NULL) {
    free(input);
  }
  if (tokens != NULL) {
    for (int i = 0; i < MAX_TOKENS; i++) {
      free(tokens[i]);
    }
    free(tokens);
  }
}

void print_cwd(char *cwd, char *input, char **tokens) {
  if (getcwd(cwd, PATH_MAX) == NULL) {
    fprintf(stderr, "Error: Could not retrieve directory. %s.\n", strerror(errno));
    free_arrays(cwd, input, tokens);
    exit(EXIT_FAILURE);
  } else if (signal_val != 0) {
    signal_val = 0;
    printf("\n[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
  } else {
    printf("[%s%s%s]$ ", BRIGHTBLUE, cwd, DEFAULT);
  }
}

void get_input(char *cwd, char *input, char **tokens) {
  if (fgets(input, MAX_COMMAND_LEN, stdin) == NULL) {
    if (signal_val != 0) {
      signal_val = 0;
    } else {
      fprintf(stderr, "Error: Could not read from stdin. %s.\n", strerror(errno));
      free_arrays(cwd, input, tokens);
      exit(EXIT_FAILURE);
    }
  }
}

void malloc_arrays(char **cwd, char **input, char ***tokens) {
  *cwd = (char *)malloc(PATH_MAX * sizeof(char));
  if (*cwd == NULL) {
    fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  memset(*cwd, '\0', PATH_MAX * sizeof(char));
  *input = (char *)malloc(MAX_COMMAND_LEN * sizeof(char));
  if (*input == NULL) {
    fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
    free_arrays(*cwd, NULL, NULL);
    exit(EXIT_FAILURE);
  }
  memset(*input, '\0', MAX_COMMAND_LEN * sizeof(char));
  *tokens = (char **)malloc(MAX_TOKENS * sizeof(char *));

  if (*tokens == NULL) {
    fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
    free_arrays(*cwd, *input, NULL);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < MAX_TOKENS; i++) {
    (*tokens)[i] = (char *)malloc(MAX_COMMAND_LEN * sizeof(char));
    if ((*tokens)[i] == NULL) {
      fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
      free_arrays(*cwd, *input, *tokens);
      exit(EXIT_FAILURE);
    }
    memset((*tokens)[i], '\0', MAX_COMMAND_LEN * sizeof(char));
  }
}

void alloc_arrays(char **input, char ***tokens) {
  if (tokens == NULL) {
    memset(*input, '\0', MAX_COMMAND_LEN * sizeof(char));
  } else {
    for (int i = 0; i < MAX_TOKENS; i++) {
      memset((*tokens)[i], '\0', MAX_COMMAND_LEN * sizeof(char));
    }
  }
}

void tokenize_strings(char *cwd, char *input, char **tokens) {
  int i_index = 0;
  int s_index = 0;
  int t_index = 0;
  char quote_seen = 'N';

  while (input[i_index] != '\n') {
    if (input[i_index] == '"') {
      if (quote_seen == 'N') {
        quote_seen == 'Y';
      } else {
        if (input[i_index+1] == '"') {
          i_index++;
        } else {
          quote_seen = 'N';
        }
      }
    } else if (quote_seen == 'Y' || (quote_seen == 'N' && input[i_index] != ' ')) {
      tokens[i_index][s_index] = input[i_index];
      s_index++;
    } else {
      t_index++;
      s_index = 0;
    }
    i_index++;
  }
  if (quote_seen == 'Y') {
    fprintf(stderr, "Error: Malformed command. %s.\n", strerror(errno));
    free_arrays(cwd, input, tokens);
    exit(EXIT_FAILURE);
  }
}

void change_directory(char *cwd, char *input, char **tokens) {
  char *path;
  char null_path = tokens[1][0];
  char real_path[PATH_MAX];
  struct stat path_stat;

  if (null_path == '~') {
    char *home = getenv("HOME");
    int home_len = strlen(home);
    int path_len = strlen(path);

    for (int i = path_len; i > 0; i--) {
      path[i + home_len - 1] = path[i];
    }
    for (int i = 0; i < home_len; i++) {
      path[i] = home[i];
    }
  }
  if (null_path == '\0' || strcmp(path, "~") == 0) {
    path = getenv("HOME");
  }

  if (realpath(path, real_path) == NULL) {
    fprintf(stderr, "Error: Cannot get full path of directory '%s'. %s.\n", path, strerror(errno));
    free_arrays(cwd, input, tokens);
    exit(EXIT_FAILURE);
  }

  if (stat(real_path, &path_stat) == -1) {
    fprintf(stderr, "Error: Cannot stat directory '%s'. %s.\n", real_path, strerror(errno));
    free_arrays(cwd, input, tokens);
    exit(EXIT_FAILURE);
  }

  if (S_ISDIR(path_stat.st_mode) == 0) {
    fprintf(stderr, "Error: '%s' is not a directory path. %s.\n", real_path, strerror(errno));
    free_arrays(cwd, input, tokens);
    exit(EXIT_FAILURE);
  }

  if (chdir(real_path) == -1) {
    fprintf(stderr, "Error: Cannot change current directory to '%s'. %s.\n", real_path, strerror(errno));
    free_arrays(cwd, input, tokens);
    exit(EXIT_FAILURE);
  }
}

void remaining_commands(char *cwd, char *input, char **tokens) {
  pid_t pid = fork();
  if (pid == -1) {
    fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
    free_arrays(cwd, input, tokens);
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    int t_index = 0;
    char *child_tokens[MAX_TOKENS];
    while (tokens[t_index][0] != '\0') {
      child_tokens[t_index] = tokens[t_index];
      t_index++;
    }
    child_tokens[t_index] = NULL;
    if (execvp(child_tokens[0], child_tokens) == -1) {
      fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
      free_arrays(cwd, input, tokens);
      exit(EXIT_FAILURE);
    }
  } else {
    int status;
    pid_t wait_pid = waitpid(pid, &status, WUNTRACED | WCONTINUED);
    if (wait_pid == -1) {
      if (signal_val != 0) {
        signal_val = 0;
      } else {
        fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
        free_arrays(cwd, input, tokens);
        exit(EXIT_FAILURE);
      }
    }
  }
}

void check_tokens(char *cwd, char *input, char **tokens) {
  if (strcmp(tokens[0], "exit") == 0) {
    free_arrays(cwd, input, tokens);
    exit(EXIT_SUCCESS);
  } else if (strcmp(tokens[0], "cd") == 0) {
    change_directory(cwd, input, tokens);
  } else if (tokens[0][0] != '\0') {
    remaining_commands(cwd, input, tokens);
  }
}

int main(int argc, char **argv) {
  signal_check();
  arg_check(argc);
  int exit_status = 0;
  char *cwd;
  char *input;
  char **tokens;
  malloc_arrays(&cwd, &input, &tokens);
  while (exit_status == 0) {
    print_cwd(cwd, input, tokens);
    alloc_arrays(&input, NULL);
    get_input(cwd, input, tokens);
    alloc_arrays(NULL, &tokens);
    tokenize_strings(cwd, input, tokens);
    check_tokens(cwd, input, tokens);
  }
  free_arrays(cwd, input, tokens);
  return EXIT_SUCCESS;
}