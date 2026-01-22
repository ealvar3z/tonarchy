#include "build_iso.h"
#include <stdarg.h>

static FILE *log_file = NULL;

void logger_init(const char *log_path) {
    log_file = fopen(log_path, "a");
    if (log_file) {
        time_t now = time(NULL);
        char *timestamp = ctime(&now);
        timestamp[strlen(timestamp) - 1] = '\0';
        fprintf(log_file, "\n=== Tonarchy ISO Build Log - %s ===\n", timestamp);
        fflush(log_file);
    }
}

void logger_close(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[INFO] ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);

    if (log_file) {
        va_start(args, fmt);
        fprintf(log_file, "[INFO] ");
        vfprintf(log_file, fmt, args);
        fprintf(log_file, "\n");
        fflush(log_file);
        va_end(args);
    }
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);

    if (log_file) {
        va_start(args, fmt);
        fprintf(log_file, "[ERROR] ");
        vfprintf(log_file, fmt, args);
        fprintf(log_file, "\n");
        fflush(log_file);
        va_end(args);
    }
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[WARN] ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);

    if (log_file) {
        va_start(args, fmt);
        fprintf(log_file, "[WARN] ");
        vfprintf(log_file, fmt, args);
        fprintf(log_file, "\n");
        fflush(log_file);
        va_end(args);
    }
}

int run_command(const char *cmd) {
    log_info("Running: %s", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        log_error("Command failed with code %d: %s", ret, cmd);
        return 0;
    }
    return 1;
}

int run_command_in_container(const char *cmd, const Build_Config *config) {
    char container_cmd[CMD_MAX_LEN];

    if (config->container_type == CONTAINER_DISTROBOX) {
        snprintf(container_cmd, sizeof(container_cmd),
                 "distrobox enter %s -- sh -c '%s'",
                 config->distrobox_name, cmd);
    } else if (config->container_type == CONTAINER_PODMAN) {
        snprintf(container_cmd, sizeof(container_cmd),
                 "podman run --rm --privileged "
                 "-v '%s:/src' "
                 "-v '%s:/profile' "
                 "-v '%s:/out' "
                 "-v '%s:/work' "
                 "docker.io/archlinux:latest sh -c '%s'",
                 config->tonarchy_src,
                 config->iso_profile,
                 config->out_dir,
                 config->work_dir,
                 cmd);
    } else {
        return run_command(cmd);
    }

    return run_command(container_cmd);
}

int create_directory(const char *path, mode_t mode) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return 1;
    }

    if (mkdir(path, mode) != 0) {
        log_error("Failed to create directory: %s", path);
        return 0;
    }
    return 1;
}

int detect_container_runtime(void) {
    if (system("command -v podman >/dev/null 2>&1") == 0) {
        return 1;
    }
    if (system("command -v distrobox >/dev/null 2>&1") == 0) {
        return 1;
    }
    return 0;
}

int check_distrobox_exists(const char *name) {
    char cmd[CMD_MAX_LEN];
    snprintf(cmd, sizeof(cmd), "distrobox list | grep -q '%s'", name);
    return system(cmd) == 0;
}

int build_tonarchy_static(const Build_Config *config) {
    log_info("Building tonarchy static binary...");

    char cmd[CMD_MAX_LEN];

    if (config->use_container && config->container_type == CONTAINER_PODMAN) {
        log_info("Building static binary in podman container...");
        snprintf(cmd, sizeof(cmd),
                 "sudo podman run --rm "
                 "-v '%s:/src' "
                 "docker.io/archlinux:latest "
                 "sh -c 'pacman -Sy --noconfirm musl gcc && "
                 "cd /src && rm -f tonarchy tonarchy-static && "
                 "musl-gcc -std=c23 -Wall -Wextra -O2 -static src/tonarchy.c -o tonarchy-static'",
                 config->tonarchy_src);
        if (!run_command(cmd)) {
            log_error("Failed to build tonarchy-static in podman");
            return 0;
        }
    } else if (config->use_container && config->container_type == CONTAINER_DISTROBOX) {
        snprintf(cmd, sizeof(cmd),
                 "sudo pacman -S --noconfirm --needed musl && "
                 "cd '%s' && make clean && make static CC=musl-gcc",
                 config->tonarchy_src);
        if (!run_command_in_container(cmd, config)) {
            log_error("Failed to build tonarchy-static in distrobox");
            return 0;
        }
    } else {
        snprintf(cmd, sizeof(cmd),
                 "cd '%s' && make clean && make static CC=musl-gcc",
                 config->tonarchy_src);
        if (!run_command(cmd)) {
            log_error("Failed to build tonarchy-static");
            return 0;
        }
    }

    char binary_path[PATH_MAX_LEN];
    snprintf(binary_path, sizeof(binary_path), "%s/tonarchy-static", config->tonarchy_src);

    struct stat st;
    if (stat(binary_path, &st) != 0) {
        log_error("tonarchy-static binary not found at %s", binary_path);
        return 0;
    }

    log_info("Built tonarchy-static successfully");
    return 1;
}

int clean_airootfs(const Build_Config *config) {
    log_info("Cleaning airootfs...");

    char cmd[CMD_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "sudo rm -rf '%s/airootfs/usr'", config->iso_profile);
    if (!run_command(cmd)) {
        log_warn("Failed to clean airootfs/usr");
    }

    snprintf(cmd, sizeof(cmd), "sudo rm -rf '%s/airootfs/root/tonarchy'", config->iso_profile);
    if (!run_command(cmd)) {
        log_warn("Failed to clean airootfs/root/tonarchy");
    }

    return 1;
}

int clean_work_dir(const Build_Config *config) {
    log_info("Cleaning work directory...");

    char cmd[CMD_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "sudo umount -R '%s' 2>/dev/null || true", config->work_dir);
    run_command(cmd);

    snprintf(cmd, sizeof(cmd), "sudo rm -rf '%s'", config->work_dir);
    if (!run_command(cmd)) {
        log_error("Failed to remove work directory: %s", config->work_dir);
        return 0;
    }

    sync();
    sleep(1);

    return 1;
}

int prepare_airootfs(const Build_Config *config) {
    log_info("Preparing airootfs...");

    char cmd[CMD_MAX_LEN];
    char src_path[PATH_MAX_LEN];
    char dest_path[PATH_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/airootfs/usr/local/bin'", config->iso_profile);
    if (!run_command(cmd)) {
        log_error("Failed to create airootfs/usr/local/bin");
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/airootfs/usr/share'", config->iso_profile);
    if (!run_command(cmd)) {
        log_error("Failed to create airootfs/usr/share");
        return 0;
    }

    snprintf(src_path, sizeof(src_path), "%s/tonarchy-static", config->tonarchy_src);
    snprintf(dest_path, sizeof(dest_path), "%s/airootfs/usr/local/bin/tonarchy", config->iso_profile);
    snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", src_path, dest_path);
    if (!run_command(cmd)) {
        log_error("Failed to copy tonarchy binary");
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "chmod 755 '%s'", dest_path);
    if (!run_command(cmd)) {
        log_error("Failed to set permissions on tonarchy binary");
        return 0;
    }

    snprintf(src_path, sizeof(src_path), "%s/assets", config->tonarchy_src);
    snprintf(dest_path, sizeof(dest_path), "%s/airootfs/usr/share/tonarchy", config->iso_profile);
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", dest_path);
    if (!run_command(cmd)) {
        log_error("Failed to create tonarchy share directory");
        return 0;
    }
    snprintf(cmd, sizeof(cmd), "cp -r '%s'/* '%s'", src_path, dest_path);
    if (!run_command(cmd)) {
        log_error("Failed to copy tonarchy config files");
        return 0;
    }

    snprintf(dest_path, sizeof(dest_path), "%s/airootfs/usr/share/wallpapers", config->iso_profile);
    snprintf(cmd, sizeof(cmd), "cp -r '%s/wallpapers' '%s'", src_path, dest_path);
    if (!run_command(cmd)) {
        log_warn("Failed to copy wallpapers");
    }

    log_info("Setting proper ownership for airootfs...");
    snprintf(cmd, sizeof(cmd), "sudo chown -R root:root '%s/airootfs/usr'", config->iso_profile);
    if (!run_command(cmd)) {
        log_error("Failed to set ownership on airootfs");
        return 0;
    }

    return 1;
}

int run_mkarchiso(const Build_Config *config) {
    log_info("Building ISO with mkarchiso...");

    if (!create_directory(config->out_dir, 0755)) {
        return 0;
    }

    if (!create_directory(config->work_dir, 0755)) {
        return 0;
    }

    char cmd[CMD_MAX_LEN];
    snprintf(cmd, sizeof(cmd), "sudo mkarchiso -v -w '%s' -o '%s' '%s'",
             config->work_dir, config->out_dir, config->iso_profile);

    if (!run_command(cmd)) {
        log_error("mkarchiso failed");
        return 0;
    }

    return 1;
}

int run_mkarchiso_in_container(const Build_Config *config) {
    log_info("Building ISO with mkarchiso in container...");

    if (!create_directory(config->out_dir, 0755)) {
        return 0;
    }

    if (!create_directory(config->work_dir, 0755)) {
        return 0;
    }

    char cmd[CMD_MAX_LEN];

    log_info("Setting up container policy...");
    run_command("sudo mkdir -p /etc/containers");
    run_command("echo '{\"default\":[{\"type\":\"insecureAcceptAnything\"}]}' | sudo tee /etc/containers/policy.json > /dev/null");

    snprintf(cmd, sizeof(cmd),
             "sudo podman run --rm --privileged "
             "-v '%s:/profile' "
             "-v '%s:/out' "
             "-v '%s:/work' "
             "docker.io/archlinux:latest "
             "sh -c 'pacman -Sy --noconfirm archiso && mkarchiso -v -w /work -o /out /profile'",
             config->iso_profile,
             config->out_dir,
             config->work_dir);

    if (!run_command(cmd)) {
        log_error("mkarchiso in container failed");
        return 0;
    }

    return 1;
}

const char *find_latest_iso(const char *out_dir) {
    static char iso_path[PATH_MAX_LEN];
    char cmd[CMD_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "ls -t '%s'/*.iso 2>/dev/null | head -n1", out_dir);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return NULL;
    }

    if (fgets(iso_path, sizeof(iso_path), fp) != NULL) {
        iso_path[strcspn(iso_path, "\n")] = 0;
        pclose(fp);
        return iso_path;
    }

    pclose(fp);
    return NULL;
}

static void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\nOptions:\n");
    printf("  --iso-profile PATH    Path to ISO profile directory (default: ./iso)\n");
    printf("  --out-dir PATH        Output directory for ISO (default: ./out)\n");
    printf("  --container [TYPE]    Build using container (podman or distrobox)\n");
    printf("  --distrobox NAME      Distrobox container name (default: arch)\n");
    printf("  -h, --help            Show this help message\n");
}

static int parse_args(int argc, char *argv[], Build_Config *config) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--iso-profile") == 0 && i + 1 < argc) {
            snprintf(config->iso_profile, sizeof(config->iso_profile), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--out-dir") == 0 && i + 1 < argc) {
            snprintf(config->out_dir, sizeof(config->out_dir), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--container") == 0) {
            config->use_container = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                i++;
                if (strcmp(argv[i], "podman") == 0) {
                    config->container_type = CONTAINER_PODMAN;
                } else if (strcmp(argv[i], "distrobox") == 0) {
                    config->container_type = CONTAINER_DISTROBOX;
                } else {
                    log_error("Unknown container type: %s", argv[i]);
                    return 0;
                }
            } else {
                config->container_type = CONTAINER_PODMAN;
            }
        } else if (strcmp(argv[i], "--distrobox") == 0 && i + 1 < argc) {
            snprintf(config->distrobox_name, sizeof(config->distrobox_name), "%s", argv[++i]);
            config->use_container = true;
            config->container_type = CONTAINER_DISTROBOX;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else {
            log_error("Unknown option: %s", argv[i]);
            print_usage(argv[0]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    logger_init("/tmp/build_iso.log");

    log_info("Tonarchy ISO Builder starting...");

    Build_Config config = {
        .work_dir = "/tmp/tonarchy_iso_work",
        .distrobox_name = "arch",
        .container_type = CONTAINER_NONE,
        .use_container = false
    };

    if (getcwd(config.tonarchy_src, sizeof(config.tonarchy_src)) == NULL) {
        log_error("Failed to get current directory");
        return 1;
    }

    snprintf(config.iso_profile, sizeof(config.iso_profile), "%s/iso", config.tonarchy_src);
    snprintf(config.out_dir, sizeof(config.out_dir), "%s/out", config.tonarchy_src);

    if (!parse_args(argc, argv, &config)) {
        logger_close();
        return 1;
    }

    log_info("Tonarchy source: %s", config.tonarchy_src);
    log_info("ISO profile: %s", config.iso_profile);
    log_info("Work directory: %s", config.work_dir);
    log_info("Output directory: %s", config.out_dir);

    if (config.use_container) {
        log_info("Container mode: %s",
                 config.container_type == CONTAINER_PODMAN ? "podman" : "distrobox");
        if (config.container_type == CONTAINER_DISTROBOX) {
            log_info("Distrobox name: %s", config.distrobox_name);
        }
    }

    if (!build_tonarchy_static(&config)) {
        log_error("Build failed");
        logger_close();
        return 1;
    }

    if (!clean_airootfs(&config)) {
        log_error("Failed to clean airootfs");
        logger_close();
        return 1;
    }

    if (!clean_work_dir(&config)) {
        log_error("Failed to clean work directory");
        logger_close();
        return 1;
    }

    if (!prepare_airootfs(&config)) {
        log_error("Failed to prepare airootfs");
        logger_close();
        return 1;
    }

    int build_result;
    if (config.use_container && config.container_type == CONTAINER_PODMAN) {
        build_result = run_mkarchiso_in_container(&config);
    } else {
        build_result = run_mkarchiso(&config);
    }

    if (!build_result) {
        log_error("Failed to build ISO");
        logger_close();
        return 1;
    }

    if (!clean_work_dir(&config)) {
        log_warn("Failed to clean work directory after build");
    }

    log_info("Syncing filesystem...");
    sync();
    sleep(2);

    const char *iso_path = find_latest_iso(config.out_dir);
    if (iso_path) {
        log_info("===================================");
        log_info("ISO created successfully!");
        log_info("Location: %s", iso_path);
        log_info("Test with: make test");
        log_info("===================================");
    } else {
        log_error("ISO was built but could not be found in output directory");
        logger_close();
        return 1;
    }

    logger_close();
    return 0;
}
