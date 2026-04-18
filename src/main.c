#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define ZEROS_VERSION "1.0"
#define MAX_INPUT 1024
#define MAX_PROMPT 128

static char install_root[PATH_MAX] = {0};
static char current_dir[PATH_MAX] = {0};
static char prompt_name[MAX_PROMPT] = "ZerOS";
static char account_name[64] = "admin";
static char account_password[64] = "";

static void trim_newline(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

static void read_line(const char *label, char *buf, size_t cap) {
    if (label) {
        printf("%s", label);
    }
    if (!fgets(buf, (int)cap, stdin)) {
        buf[0] = '\0';
        return;
    }
    trim_newline(buf);
}

static void zeros_banner(void) {
    printf("\033[42;37m");
    printf("\n");
    printf("███████╗███████╗██████╗  ██████╗ ███████╗\n");
    printf("╚══███╔╝██╔════╝██╔══██╗██╔═══██╗██╔════╝\n");
    printf("  ███╔╝ █████╗  ██████╔╝██║   ██║███████╗\n");
    printf(" ███╔╝  ██╔══╝  ██╔══██╗██║   ██║╚════██║\n");
    printf("███████╗███████╗██║  ██║╚██████╔╝███████║\n");
    printf("╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝\n");
    printf("ZerOS %s | Введи help чтобы узнать команды\n\n", ZEROS_VERSION);
    printf("\033[0m");
}

static int recursive_delete(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            recursive_delete(full_path);
            rmdir(full_path);
        } else {
            unlink(full_path);
        }
    }

    closedir(dir);
    return 0;
}

static void initialize_zfm(const char *root) {
    char zfm_dir[PATH_MAX];
    snprintf(zfm_dir, sizeof(zfm_dir), "%s/ZFM", root);
    mkdir(zfm_dir, 0755);

    char users_dir[PATH_MAX];
    snprintf(users_dir, sizeof(users_dir), "%s/users", zfm_dir);
    mkdir(users_dir, 0755);

    char docs_dir[PATH_MAX];
    snprintf(docs_dir, sizeof(docs_dir), "%s/docs", zfm_dir);
    mkdir(docs_dir, 0755);

    char sys_dir[PATH_MAX];
    snprintf(sys_dir, sizeof(sys_dir), "%s/system", zfm_dir);
    mkdir(sys_dir, 0755);

    char boot_txt[PATH_MAX];
    snprintf(boot_txt, sizeof(boot_txt), "%s/system/boot.log", zfm_dir);
    FILE *f = fopen(boot_txt, "w");
    if (f) {
        fprintf(f, "ZerOS %s initialized with ZFM\n", ZEROS_VERSION);
        fclose(f);
    }
}

static bool run_installation(void) {
    char answer[16];
    printf("Добро пожаловать в ZerOS Installer.\n");
    printf("Система не установлена. Установить сейчас? (y/n): ");
    if (!fgets(answer, sizeof(answer), stdin)) {
        return false;
    }

    if (tolower(answer[0]) != 'y') {
        printf("Установка отменена. Завершение.\n");
        return false;
    }

    char target[PATH_MAX];
    read_line("Выберите диск/директорию для установки (пример: ./virtual_disk): ", target, sizeof(target));
    if (target[0] == '\0') {
        printf("Неверная директория.\n");
        return false;
    }

    printf("ВНИМАНИЕ: Все файлы в %s будут уничтожены. Продолжить? (y/n): ", target);
    if (!fgets(answer, sizeof(answer), stdin)) {
        return false;
    }
    if (tolower(answer[0]) != 'y') {
        printf("Установка отменена пользователем.\n");
        return false;
    }

    mkdir(target, 0755);
    recursive_delete(target);

    initialize_zfm(target);

    strncpy(install_root, target, sizeof(install_root) - 1);
    snprintf(current_dir, sizeof(current_dir), "%s/ZFM", install_root);

    printf("Создайте учетную запись.\n");
    read_line("Имя пользователя: ", account_name, sizeof(account_name));
    if (account_name[0] == '\0') {
        strncpy(account_name, "admin", sizeof(account_name) - 1);
    }

    char set_password[16];
    read_line("Задать пароль? (y/n): ", set_password, sizeof(set_password));
    if (tolower(set_password[0]) == 'y') {
        read_line("Введите пароль: ", account_password, sizeof(account_password));
    }

    char user_file[PATH_MAX];
    snprintf(user_file, sizeof(user_file), "%s/ZFM/users/%s.user", install_root, account_name);
    FILE *u = fopen(user_file, "w");
    if (u) {
        fprintf(u, "username=%s\npassword=%s\n", account_name, account_password);
        fclose(u);
    }

    printf("Установка завершена успешно.\n");
    return true;
}

static void show_help(void) {
    printf("Доступные команды:\n");
    printf("  help                  - список команд\n");
    printf("  clear | clean         - очистка экрана\n");
    printf("  version               - версия системы\n");
    printf("  echo <text>           - вывести текст\n");
    printf("  prompt <name>         - сменить приглашение\n");
    printf("  time                  - текущее время\n");
    printf("  date                  - текущая дата\n");
    printf("  month                 - текущий месяц\n");
    printf("  season                - текущее время года\n");
    printf("  hw                    - комплектующие системы\n");
    printf("  mem                   - память\n");
    printf("  ls                    - показать файлы\n");
    printf("  pwd                   - текущая директория\n");
    printf("  cd <dir>              - перейти в директорию\n");
    printf("  mkdir <dir>           - создать директорию\n");
    printf("  touch <file>          - создать пустой файл\n");
    printf("  cat <file>            - показать файл\n");
    printf("  write <file> <text>   - записать строку в файл\n");
    printf("  rm <file>             - удалить файл\n");
    printf("  rmdir <dir>           - удалить пустую директорию\n");
    printf("  notepad <file>        - простой блокнот\n");
    printf("  game guess            - игра Угадай число\n");
    printf("  game coin             - игра Монетка\n");
    printf("  ide                   - мини IDE для C#\n");
    printf("  exit                  - выход\n");
}

static void build_path(char *dst, size_t cap, const char *name) {
    if (!name || name[0] == '\0') {
        snprintf(dst, cap, "%s", current_dir);
        return;
    }
    if (name[0] == '/') {
        snprintf(dst, cap, "%s", name);
    } else {
        snprintf(dst, cap, "%s/%s", current_dir, name);
    }
}

static void cmd_ls(void) {
    DIR *dir = opendir(current_dir);
    if (!dir) {
        printf("Ошибка ls: %s\n", strerror(errno));
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            printf("%s\n", entry->d_name);
        }
    }
    closedir(dir);
}

static void cmd_cd(const char *arg) {
    if (!arg) {
        return;
    }
    char path[PATH_MAX];
    build_path(path, sizeof(path), arg);

    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        strncpy(current_dir, path, sizeof(current_dir) - 1);
    } else {
        printf("Директория не найдена: %s\n", arg);
    }
}

static void cmd_touch(const char *arg) {
    if (!arg) {
        return;
    }
    char path[PATH_MAX];
    build_path(path, sizeof(path), arg);
    FILE *f = fopen(path, "a");
    if (!f) {
        printf("Ошибка touch: %s\n", strerror(errno));
        return;
    }
    fclose(f);
}

static void cmd_cat(const char *arg) {
    if (!arg) return;
    char path[PATH_MAX];
    build_path(path, sizeof(path), arg);
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Ошибка cat: %s\n", strerror(errno));
        return;
    }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        fputs(line, stdout);
    }
    fclose(f);
}

static void cmd_write(const char *filename, const char *text) {
    if (!filename || !text) return;
    char path[PATH_MAX];
    build_path(path, sizeof(path), filename);
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Ошибка write: %s\n", strerror(errno));
        return;
    }
    fprintf(f, "%s\n", text);
    fclose(f);
}

static void cmd_notepad(const char *filename) {
    if (!filename) {
        printf("Использование: notepad <file>\n");
        return;
    }
    char path[PATH_MAX];
    build_path(path, sizeof(path), filename);
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Ошибка notepad: %s\n", strerror(errno));
        return;
    }

    printf("Блокнот: вводите текст, ':wq' для сохранения и выхода.\n");
    char line[512];
    while (true) {
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        trim_newline(line);
        if (strcmp(line, ":wq") == 0) {
            break;
        }
        fprintf(f, "%s\n", line);
    }
    fclose(f);
}

static void game_guess(void) {
    srand((unsigned)time(NULL));
    int secret = (rand() % 100) + 1;
    char in[64];
    printf("Угадай число от 1 до 100\n");
    while (true) {
        read_line("> ", in, sizeof(in));
        int guess = atoi(in);
        if (guess < secret) {
            printf("Больше\n");
        } else if (guess > secret) {
            printf("Меньше\n");
        } else {
            printf("Верно!\n");
            return;
        }
    }
}

static void game_coin(void) {
    srand((unsigned)time(NULL));
    printf("Монетка: %s\n", (rand() % 2) ? "ОРЕЛ" : "РЕШКА");
}

static void cmd_hw(void) {
    struct utsname un;
    if (uname(&un) == 0) {
        printf("Система: %s %s\n", un.sysname, un.release);
        printf("Архитектура: %s\n", un.machine);
    }

    FILE *cpu = fopen("/proc/cpuinfo", "r");
    if (cpu) {
        char line[256];
        while (fgets(line, sizeof(line), cpu)) {
            if (strncmp(line, "model name", 10) == 0) {
                printf("CPU: %s", strchr(line, ':') + 2);
                break;
            }
        }
        fclose(cpu);
    }
}

static void cmd_mem(void) {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        printf("Ошибка получения памяти\n");
        return;
    }
    unsigned long total_mb = (info.totalram * info.mem_unit) / (1024UL * 1024UL);
    unsigned long free_mb = (info.freeram * info.mem_unit) / (1024UL * 1024UL);
    printf("RAM total: %lu MB\n", total_mb);
    printf("RAM free : %lu MB\n", free_mb);
}

static void cmd_time_related(const char *kind) {
    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);
    if (!tmv) return;

    if (strcmp(kind, "time") == 0) {
        printf("%02d:%02d:%02d\n", tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
    } else if (strcmp(kind, "date") == 0) {
        printf("%04d-%02d-%02d\n", tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday);
    } else if (strcmp(kind, "month") == 0) {
        static const char *months[] = {"Январь", "Февраль", "Март", "Апрель", "Май", "Июнь", "Июль", "Август", "Сентябрь", "Октябрь", "Ноябрь", "Декабрь"};
        printf("%s\n", months[tmv->tm_mon]);
    } else if (strcmp(kind, "season") == 0) {
        int m = tmv->tm_mon + 1;
        if (m == 12 || m <= 2) printf("Зима\n");
        else if (m <= 5) printf("Весна\n");
        else if (m <= 8) printf("Лето\n");
        else printf("Осень\n");
    }
}

static void cmd_ide(void) {
    char proj[128];
    read_line("Название C# проекта: ", proj, sizeof(proj));
    if (proj[0] == '\0') {
        printf("Пустое имя проекта.\n");
        return;
    }

    char dir[PATH_MAX];
    build_path(dir, sizeof(dir), proj);
    mkdir(dir, 0755);

    char cs[PATH_MAX];
    snprintf(cs, sizeof(cs), "%s/Program.cs", dir);
    FILE *f = fopen(cs, "w");
    if (!f) {
        printf("Ошибка IDE: %s\n", strerror(errno));
        return;
    }
    fprintf(f,
            "using System;\n\n"
            "class Program {\n"
            "    static void Main() {\n"
            "        Console.WriteLine(\"Hello from ZerOS IDE!\");\n"
            "    }\n"
            "}\n");
    fclose(f);

    printf("Создан шаблон C# приложения: %s\n", cs);
    printf("Попытаться собрать через dotnet? (y/n): ");
    char ans[8];
    if (!fgets(ans, sizeof(ans), stdin)) return;
    if (tolower(ans[0]) == 'y') {
        char cmd[PATH_MAX + 64];
        snprintf(cmd, sizeof(cmd), "cd '%s' && dotnet new console --force >/dev/null 2>&1 && dotnet build", dir);
        int rc = system(cmd);
        if (rc == 0) printf("Сборка завершена успешно.\n");
        else printf("dotnet не найден или сборка провалилась.\n");
    }
}

static void command_loop(void) {
    zeros_banner();
    char input[MAX_INPUT];

    while (true) {
        printf("%s:%s$ ", prompt_name, current_dir);
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        trim_newline(input);
        if (input[0] == '\0') {
            continue;
        }

        char *cmd = strtok(input, " ");
        char *arg1 = strtok(NULL, " ");
        char *arg2 = strtok(NULL, "");

        if (strcmp(cmd, "help") == 0) show_help();
        else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "clean") == 0) printf("\033[2J\033[H");
        else if (strcmp(cmd, "version") == 0) printf("ZerOS version %s\n", ZEROS_VERSION);
        else if (strcmp(cmd, "echo") == 0) printf("%s\n", arg1 ? arg1 : "");
        else if (strcmp(cmd, "prompt") == 0 && arg1) strncpy(prompt_name, arg1, sizeof(prompt_name) - 1);
        else if (strcmp(cmd, "time") == 0 || strcmp(cmd, "date") == 0 || strcmp(cmd, "month") == 0 || strcmp(cmd, "season") == 0) cmd_time_related(cmd);
        else if (strcmp(cmd, "hw") == 0) cmd_hw();
        else if (strcmp(cmd, "mem") == 0) cmd_mem();
        else if (strcmp(cmd, "ls") == 0) cmd_ls();
        else if (strcmp(cmd, "pwd") == 0) printf("%s\n", current_dir);
        else if (strcmp(cmd, "cd") == 0) cmd_cd(arg1);
        else if (strcmp(cmd, "mkdir") == 0 && arg1) {
            char path[PATH_MAX];
            build_path(path, sizeof(path), arg1);
            if (mkdir(path, 0755) != 0) printf("Ошибка mkdir: %s\n", strerror(errno));
        }
        else if (strcmp(cmd, "touch") == 0) cmd_touch(arg1);
        else if (strcmp(cmd, "cat") == 0) cmd_cat(arg1);
        else if (strcmp(cmd, "write") == 0) cmd_write(arg1, arg2 ? arg2 : "");
        else if (strcmp(cmd, "rm") == 0 && arg1) {
            char path[PATH_MAX];
            build_path(path, sizeof(path), arg1);
            if (unlink(path) != 0) printf("Ошибка rm: %s\n", strerror(errno));
        }
        else if (strcmp(cmd, "rmdir") == 0 && arg1) {
            char path[PATH_MAX];
            build_path(path, sizeof(path), arg1);
            if (rmdir(path) != 0) printf("Ошибка rmdir: %s\n", strerror(errno));
        }
        else if (strcmp(cmd, "notepad") == 0) cmd_notepad(arg1);
        else if (strcmp(cmd, "game") == 0 && arg1) {
            if (strcmp(arg1, "guess") == 0) game_guess();
            else if (strcmp(arg1, "coin") == 0) game_coin();
            else printf("Неизвестная игра\n");
        }
        else if (strcmp(cmd, "ide") == 0) cmd_ide();
        else if (strcmp(cmd, "exit") == 0) break;
        else printf("Команда не найдена. Введи help.\n");
    }
}

int main(void) {
    if (!run_installation()) {
        return 0;
    }
    command_loop();
    return 0;
}
