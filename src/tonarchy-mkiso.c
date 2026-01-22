#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

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

#define LOG_INFO(fmt, ...) do { \
    printf("[INFO] " fmt "\n", ##__VA_ARGS__); \
    if (log_file) { \
        fprintf(log_file, "[INFO] " fmt "\n", ##__VA_ARGS__); \
        fflush(log_file); \
    } \
} while(0)

#define LOG_ERROR(fmt, ...) do { \
    fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__); \
    if (log_file) { \
        fprintf(log_file, "[ERROR] " fmt "\n", ##__VA_ARGS__); \
        fflush(log_file); \
    } \
} while(0)

#define LOG_WARN(fmt, ...) do { \
    fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__); \
    if (log_file) { \
        fprintf(log_file, "[WARN] " fmt "\n", ##__VA_ARGS__); \
        fflush(log_file); \
    } \
} while(0)

int create_directory(const char *path, mode_t mode);

int run_command(const char *cmd) {
    LOG_INFO("Running: %s", cmd);
    int ret = system(cmd);
    if (ret != 0) {
        LOG_ERROR("Command failed with code %d: %s", ret, cmd);
        return 0;
    }
    return 1;
}

int create_directory(const char *path, mode_t mode) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return 1;
    }

    if (mkdir(path, mode) != 0) {
        LOG_ERROR("Failed to create directory: %s", path);
        return 0;
    }
    return 1;
}

int build_tonarchy_static(const char *tonarchy_src) {
    LOG_INFO("Building tonarchy static binary...");

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cd '%s' && make clean && make static CC=musl-gcc", tonarchy_src);

    if (!run_command(cmd)) {
        LOG_ERROR("Failed to build tonarchy-static");
        return 0;
    }

    char binary_path[512];
    snprintf(binary_path, sizeof(binary_path), "%s/tonarchy-static", tonarchy_src);

    struct stat st;
    if (stat(binary_path, &st) != 0) {
        LOG_ERROR("tonarchy-static binary not found at %s", binary_path);
        return 0;
    }

    LOG_INFO("Built tonarchy-static successfully");
    return 1;
}

int clean_airootfs(const char *iso_profile) {
    LOG_INFO("Cleaning airootfs...");

    char cmd[512];

    snprintf(cmd, sizeof(cmd), "sudo rm -rf '%s/airootfs/usr'", iso_profile);
    if (!run_command(cmd)) {
        LOG_WARN("Failed to clean airootfs/usr");
    }

    snprintf(cmd, sizeof(cmd), "sudo rm -rf '%s/airootfs/root/tonarchy'", iso_profile);
    if (!run_command(cmd)) {
        LOG_WARN("Failed to clean airootfs/root/tonarchy");
    }

    return 1;
}

int clean_work_dir(const char *work_dir) {
    LOG_INFO("Cleaning work directory...");

    char cmd[512];

    snprintf(cmd, sizeof(cmd), "sudo umount -R '%s' 2>/dev/null || true", work_dir);
    run_command(cmd);

    snprintf(cmd, sizeof(cmd), "sudo rm -rf '%s'", work_dir);
    if (!run_command(cmd)) {
        LOG_ERROR("Failed to remove work directory: %s", work_dir);
        return 0;
    }

    sync();
    sleep(1);

    return 1;
}

int prepare_airootfs(const char *iso_profile, const char *tonarchy_src) {
    LOG_INFO("Preparing airootfs...");

    char cmd[1024];
    char src_path[512];
    char dest_path[512];

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/airootfs/usr/local/bin'", iso_profile);
    if (!run_command(cmd)) {
        LOG_ERROR("Failed to create airootfs/usr/local/bin");
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/airootfs/usr/share'", iso_profile);
    if (!run_command(cmd)) {
        LOG_ERROR("Failed to create airootfs/usr/share");
        return 0;
    }

    snprintf(src_path, sizeof(src_path), "%s/tonarchy-static", tonarchy_src);
    snprintf(dest_path, sizeof(dest_path), "%s/airootfs/usr/local/bin/tonarchy", iso_profile);
    snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", src_path, dest_path);
    if (!run_command(cmd)) {
        LOG_ERROR("Failed to copy tonarchy binary");
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "chmod 755 '%s'", dest_path);
    if (!run_command(cmd)) {
        LOG_ERROR("Failed to set permissions on tonarchy binary");
        return 0;
    }

    snprintf(src_path, sizeof(src_path), "%s/assets/firefox", tonarchy_src);
    snprintf(dest_path, sizeof(dest_path), "%s/airootfs/usr/share/tonarchy", iso_profile);
    snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s'", src_path, dest_path);
    if (!run_command(cmd)) {
        LOG_ERROR("Failed to copy tonarchy config files");
        return 0;
    }

    snprintf(src_path, sizeof(src_path), "%s/assets/wallpapers", tonarchy_src);
    snprintf(dest_path, sizeof(dest_path), "%s/airootfs/usr/share/wallpapers", iso_profile);
    snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s' 2>/dev/null || true", src_path, dest_path);
    run_command(cmd);

    LOG_INFO("Setting proper ownership for airootfs...");
    snprintf(cmd, sizeof(cmd), "sudo chown -R root:root '%s/airootfs/usr'", iso_profile);
    if (!run_command(cmd)) {
        LOG_ERROR("Failed to set ownership on airootfs");
        return 0;
    }

    return 1;
}

int run_mkarchiso(const char *iso_profile, const char *work_dir, const char *out_dir) {
    LOG_INFO("Building ISO with mkarchiso...");

    if (!create_directory(out_dir, 0755)) {
        return 0;
    }

    if (!create_directory(work_dir, 0755)) {
        return 0;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "sudo mkarchiso -v -w '%s' -o '%s' '%s'", work_dir, out_dir, iso_profile);

    if (!run_command(cmd)) {
        LOG_ERROR("mkarchiso failed");
        return 0;
    }

    return 1;
}

const char* find_latest_iso(const char *out_dir) {
    static char iso_path[512];
    char cmd[1024];

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

int main(int argc, char *argv[]) {
    logger_init("/tmp/tonarchy-mkiso.log");

    LOG_INFO("Tonarchy ISO Builder starting...");

    char tonarchy_src[512];
    char iso_profile[512];
    char out_dir[512];
    const char *work_dir = "/tmp/tonarchy_iso_work";

    const char *custom_iso_profile = NULL;
    const char *custom_out_dir = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--iso-profile") == 0 && i + 1 < argc) {
            custom_iso_profile = argv[++i];
        } else if (strcmp(argv[i], "--out-dir") == 0 && i + 1 < argc) {
            custom_out_dir = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --iso-profile PATH    Path to ISO profile directory (default: ./iso)\n");
            printf("  --out-dir PATH        Output directory for ISO (default: ./out)\n");
            printf("  -h, --help            Show this help message\n");
            return 0;
        }
    }

    if (getcwd(tonarchy_src, sizeof(tonarchy_src)) == NULL) {
        LOG_ERROR("Failed to get current directory");
        return 1;
    }

    if (custom_iso_profile) {
        snprintf(iso_profile, sizeof(iso_profile), "%s", custom_iso_profile);
    } else {
        snprintf(iso_profile, sizeof(iso_profile), "%s/iso", tonarchy_src);
    }

    if (custom_out_dir) {
        snprintf(out_dir, sizeof(out_dir), "%s", custom_out_dir);
    } else {
        snprintf(out_dir, sizeof(out_dir), "%s/out", tonarchy_src);
    }

    LOG_INFO("Tonarchy source: %s", tonarchy_src);
    LOG_INFO("ISO profile: %s", iso_profile);
    LOG_INFO("Work directory: %s", work_dir);
    LOG_INFO("Output directory: %s", out_dir);

    if (!build_tonarchy_static(tonarchy_src)) {
        LOG_ERROR("Build failed");
        logger_close();
        return 1;
    }

    if (!clean_airootfs(iso_profile)) {
        LOG_ERROR("Failed to clean airootfs");
        logger_close();
        return 1;
    }

    if (!clean_work_dir(work_dir)) {
        LOG_ERROR("Failed to clean work directory");
        logger_close();
        return 1;
    }

    if (!prepare_airootfs(iso_profile, tonarchy_src)) {
        LOG_ERROR("Failed to prepare airootfs");
        logger_close();
        return 1;
    }

    if (!run_mkarchiso(iso_profile, work_dir, out_dir)) {
        LOG_ERROR("Failed to build ISO");
        logger_close();
        return 1;
    }

    if (!clean_work_dir(work_dir)) {
        LOG_WARN("Failed to clean work directory after build");
    }

    LOG_INFO("Syncing filesystem...");
    sync();
    sleep(2);

    const char *iso_path = find_latest_iso(out_dir);
    if (iso_path) {
        LOG_INFO("===================================");
        LOG_INFO("ISO created successfully!");
        LOG_INFO("Location: %s", iso_path);
        LOG_INFO("Test with: make test");
        LOG_INFO("===================================");
    } else {
        LOG_ERROR("ISO was built but could not be found in output directory");
        logger_close();
        return 1;
    }

    logger_close();
    return 0;
}
