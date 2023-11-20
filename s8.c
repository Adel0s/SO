#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <stdint.h>
#include <sys/wait.h>

#pragma pack(1)
typedef struct bmp_header_t
{
    char signature[2];
    int32_t file_size;
    int32_t reserved;
    int32_t data_offset;
    int32_t header_size;
    int32_t width;
    int32_t height;
    int16_t planes;
    int16_t bit_count;
    int32_t compression;
    int32_t img_size;
    int32_t x_pixels;
    int32_t y_pixels;
    int32_t colors_used;
    int32_t colors_important;
} bmp_header_t;
#pragma pack()

bmp_header_t bmp_header;

int open_file(const char *pathname, int o_flags, int o_mode)
{
    int file_descriptor = open(pathname, o_flags, o_mode);
    if (file_descriptor < 0)
    {
        perror("Can not open file: ");
        exit(-1);
    }

    return file_descriptor;
}

void close_file(int file_descriptor)
{
    if (close(file_descriptor) < 0)
    {
        perror("Can not close file: ");
        exit(-1);
    }
}

void get_file_stats(const char *file_name, struct stat *buffer)
{
    if (stat(file_name, buffer) < 0)
    {
        perror("Error getting file stats: ");
        exit(EXIT_FAILURE);
    }
}

void get_file_fstats(const char *file_name, struct stat *buffer, int file_descriptor)
{
    if (fstat(file_descriptor, buffer) < 0)
    {
        perror("Error getting file stats: ");
        close_file(file_descriptor);
        exit(EXIT_FAILURE);
    }
}

void get_permissions(struct stat *stats, char *user_rights, char *group_rights, char *others_rights)
{
    mode_t mode = stats->st_mode;

    // set permissions for user
    user_rights[0] = (mode & S_IRUSR) ? 'r' : '-';
    user_rights[1] = (mode & S_IWUSR) ? 'w' : '-';
    user_rights[2] = (mode & S_IXUSR) ? 'x' : '-';
    user_rights[3] = '\0';

    // set permissions for group
    group_rights[0] = (mode & S_IRGRP) ? 'r' : '-';
    group_rights[1] = (mode & S_IWGRP) ? 'w' : '-';
    group_rights[2] = (mode & S_IXGRP) ? 'x' : '-';
    group_rights[3] = '\0';

    // set permissions for others
    others_rights[0] = (mode & S_IROTH) ? 'r' : '-';
    others_rights[1] = (mode & S_IWOTH) ? 'w' : '-';
    others_rights[2] = (mode & S_IXOTH) ? 'x' : '-';
    others_rights[3] = '\0';
}

const char *get_file_name(const char *filePath)
{
    // Find the last occurrence of '/'
    const char *lastSlash = strrchr(filePath, '/');

    // Extract the file name
    const char *fileName = (lastSlash != NULL) ? lastSlash + 1 : filePath;
    return fileName;
}

void process_BMP_file(const char *filePath, int outputFile)
{
    int fd_i = open(filePath, O_RDONLY);

    if (read(fd_i, &bmp_header, sizeof(bmp_header_t)) != sizeof(bmp_header_t))
    {
        perror("Error reading BMP header: ");
        exit(EXIT_FAILURE);
    }

    int width = bmp_header.width;
    int height = bmp_header.height;

    const char *fileName = get_file_name(filePath);

    struct stat stats;
    get_file_fstats(filePath, &stats, fd_i);

    char last_modified[20];
    strftime(last_modified, 20, "%d.%m.%Y", localtime(&stats.st_mtime));

    char user_rights[4], group_rights[4], others_rights[4];
    get_permissions(&stats, user_rights, group_rights, others_rights);

    char buffer[1024];
    int buffer_length = sprintf(buffer, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %10s\ncontorul de legaturi: %ld\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n",
                                fileName,
                                height,
                                width,
                                (unsigned long)stats.st_size,
                                stats.st_uid,
                                last_modified,
                                (unsigned long)stats.st_nlink,
                                user_rights,
                                group_rights,
                                others_rights);

    //printf("Writing into %s_statistica.txt file...\n", fileName);
    write(outputFile, buffer, buffer_length);

    close_file(fd_i);
}

void convert_to_grayscale(const char *filePath, int outputFile)
{
    int fd_i = open(filePath, O_RDONLY);

    struct stat stats;
    get_file_fstats(filePath, &stats, fd_i);

    int width = bmp_header.width;
    int height = bmp_header.height;

    unsigned char pixel[3];
    int grayscale_value;

    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            if (read(fd_i, pixel, sizeof(pixel)) != sizeof(pixel))
            {
                perror("Error reading pixel: ");
                exit(EXIT_FAILURE);
            }

            grayscale_value = 0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0];
            memset(pixel, grayscale_value, sizeof(pixel));

            if (write(outputFile, pixel, sizeof(pixel)) != sizeof(pixel))
            {
                perror("Error writing pixel: ");
                exit(EXIT_FAILURE);
            }
        }
    }

    close_file(fd_i);
    close_file(outputFile);
}

void process_file(const char *filePath, int outputFile)
{
    const char *fileName = get_file_name(filePath);

    struct stat stats;
    get_file_stats(filePath, &stats);

    char last_modified[20];
    strftime(last_modified, 20, "%d.%m.%Y", localtime(&stats.st_mtime));

    char user_rights[4], group_rights[4], others_rights[4];
    get_permissions(&stats, user_rights, group_rights, others_rights);

    char buffer[1024];
    int buffer_length = sprintf(buffer, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %10s\ncontorul de legaturi: %ld\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n",
                                fileName,
                                (unsigned long)stats.st_size,
                                stats.st_uid,
                                last_modified,
                                (unsigned long)stats.st_nlink,
                                user_rights,
                                group_rights,
                                others_rights);

    write(outputFile, buffer, buffer_length);
}

void process_directory(const char *filePath, int outputFile)
{
    const char *directoryName = get_file_name(filePath);

    struct stat stats;
    get_file_stats(filePath, &stats);

    char user_rights[4], group_rights[4], others_rights[4];
    get_permissions(&stats, user_rights, group_rights, others_rights);

    char buffer[1024];
    int buffer_length = sprintf(buffer, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n",
                                directoryName,
                                stats.st_uid,
                                user_rights,
                                group_rights,
                                others_rights);

    write(outputFile, buffer, buffer_length);
}

void process_link(const char *filePath, int outputFile)
{
    const char *linkName = get_file_name(filePath);

    struct stat stats;
    if (lstat(filePath, &stats) == -1)
    {
        perror("Error getting link stats: ");
        exit(EXIT_FAILURE);
    }

    char user_rights[4], group_rights[4], others_rights[4];
    get_permissions(&stats, user_rights, group_rights, others_rights);

    char targetPath[256];
    ssize_t link_length = readlink(filePath, targetPath, sizeof(targetPath) - 1);
    if (link_length != -1)
    {
        targetPath[link_length] = '\0';
    }

    char buffer[1024];
    int buffer_length = sprintf(buffer, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier: %ld\ndrepturi de acces user legatura: %3s\ndrepturi de acces grup legatura: %3s\ndrepturi de acces altii legatura: %3s\n",
                                linkName,
                                link_length,
                                (unsigned long)stats.st_size,
                                user_rights,
                                group_rights,
                                others_rights);

    write(outputFile, buffer, buffer_length);
}

void read_directory_files(const char *dirPath, const char *outputDir)
{
    DIR *dir = opendir(dirPath);
    if (dir == NULL)
    {
        perror("Error opening directory: ");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    printf("Processing directory files...\n");

    while ((entry = readdir(dir)) != NULL)
    {
        printf("debug\n");
        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        struct stat stats;
        get_file_stats(filePath, &stats);

        // Ignore current and parent entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        int pid = fork();
        if (pid == -1)
        {
            perror("Error forking process: ");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) // Child process
        {
            char outputFileName[1024];
            snprintf(outputFileName, sizeof(outputFileName), "%s/%s_statistica.txt", outputDir, entry->d_name);

            printf("pid==0 before open\n");
            int outputFile = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            printf("pid==0 after open\n");

            if (S_ISREG(stats.st_mode))
            {
                const char *ext = strrchr(entry->d_name, '.');
                if (ext != NULL && strcmp(ext, ".bmp") == 0)
                {
                    printf("Process bmp file...\n");
                    process_BMP_file(filePath, outputFile);
                    int grayscale_pid = fork();
                    if (grayscale_pid == -1)
                    {
                        perror("Error forking grayscale process: ");
                        exit(EXIT_FAILURE);
                    }
                    else if (grayscale_pid == 0) // Grayscale child process
                    {
                        printf("grayscale\n");
                        char grayscaleFileName[1024];
                        snprintf(grayscaleFileName, sizeof(grayscaleFileName), "%s/%s_grayscale.bmp", outputDir, entry->d_name);

                        int grayscaleFile = open(grayscaleFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

                        convert_to_grayscale(filePath, grayscaleFile);

                        exit(EXIT_SUCCESS);
                    }
                }
                else
                {
                    printf("Processing regular file...\n");
                    process_file(filePath, outputFile);
                }
            }
            else if (S_ISDIR(stats.st_mode))
            {
                printf("Processing directory...\n");
                process_directory(filePath, outputFile);
            }
            else if (S_ISLNK(stats.st_mode))
            {
                printf("Processing symbolic link...\n");
                process_link(filePath, outputFile);
            }

            exit(EXIT_SUCCESS);
        }
        else if(pid > 0)
        {
            // Parent process continues to the next iteration
            int status;
            waitpid(pid, &status, 0);
            printf("S-a încheiat procesul cu pid-ul %d și codul %d\n", pid, WEXITSTATUS(status));
        }
    }

    closedir(dir);

    printf("Finished processing directory!\n");
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <input_directory> <output_directory>\n", argv[0]);
        return 1;
    }

    char *inputPath = argv[1];
    char *outputPath = argv[2];

    int o_flags = O_WRONLY | O_CREAT | O_TRUNC;
    int o_mode = S_IRUSR | S_IWUSR;

    int outputFile = open_file("statistica.txt", o_flags, o_mode);

    read_directory_files(inputPath, outputPath);

    close_file(outputFile);

    return 0;
}
