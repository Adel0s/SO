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
#include <ctype.h>

// folosim pack(1) pentru a nu lasa compilatorul sa introduca padding
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
int correct_sentences;

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
    if (lstat(file_name, buffer) < 0)
    {
        perror("Error getting file stats: ");
        exit(EXIT_FAILURE);
    }
}

void get_file_lstats(const char *file_name, struct stat *buffer)
{
    if (lstat(file_name, buffer) < 0)
    {
        perror("Error getting file lstats: ");
        exit(EXIT_FAILURE);
    }
}

void get_file_fstats(const char *file_name, struct stat *buffer, int file_descriptor)
{
    if (fstat(file_descriptor, buffer) < 0)
    {
        perror("Error getting file fstats: ");
        close_file(file_descriptor);
        exit(EXIT_FAILURE);
    }
}

void get_permissions(struct stat *stats, char *user_rights, char *group_rights, char *others_rights)
{
    mode_t mode = stats->st_mode;

    // Set permissions for user
    user_rights[0] = (mode & S_IRUSR) ? 'r' : '-';
    user_rights[1] = (mode & S_IWUSR) ? 'w' : '-';
    user_rights[2] = (mode & S_IXUSR) ? 'x' : '-';
    user_rights[3] = '\0';

    // Set permissions for group
    group_rights[0] = (mode & S_IRGRP) ? 'r' : '-';
    group_rights[1] = (mode & S_IWGRP) ? 'w' : '-';
    group_rights[2] = (mode & S_IXGRP) ? 'x' : '-';
    group_rights[3] = '\0';

    // Set permissions for others
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
    get_file_stats(filePath, &stats);

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

    printf("Writing into %s_statistica.txt file...\n\n", fileName);
    write(outputFile, buffer, buffer_length);

    close(fd_i);
}

void convert_to_grayscale(const char *filePath)
{
    printf("Converting to grayscale...\n");
    int fd_i = open(filePath, O_RDWR);
    unsigned char pixel[3];
    int width = bmp_header.width;
    int height = bmp_header.height;
    int grayscale_value;

    if (lseek(fd_i, 54, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < height * width; i++)
    {
        if (read(fd_i, pixel, 3) < 0)
        {
            perror("Error reading from file: ");
            exit(EXIT_FAILURE);
        }
        grayscale_value = 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2];
        memset(pixel, grayscale_value, sizeof(pixel));
        if (lseek(fd_i, -3, SEEK_CUR) < 0)
        {
            perror("Error moving file cursor: ");
            exit(EXIT_FAILURE);
        }
        if (write(fd_i, pixel, sizeof(pixel)) != sizeof(pixel))
        {
            perror("Error writing pixel: ");
            exit(EXIT_FAILURE);
        }
    }

    close(fd_i);
    printf("Finished converting to grayscale.\n");
}

void write_reg_file(const char *filePath, int outputFile, struct stat *stats)
{
    const char *fileName = get_file_name(filePath);

    // struct stat stats;
    // get_file_stats(filePath, &stats);

    char last_modified[20];
    strftime(last_modified, 20, "%d.%m.%Y", localtime(&(*stats).st_mtime));

    char user_rights[4], group_rights[4], others_rights[4];
    get_permissions(stats, user_rights, group_rights, others_rights);

    char buffer[1024];
    int buffer_length = sprintf(buffer, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %10s\ncontorul de legaturi: %ld\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n",
                                fileName,
                                (unsigned long)(*stats).st_size,
                                (*stats).st_uid,
                                last_modified,
                                (unsigned long)(*stats).st_nlink,
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

void process_link(const char *filePath, int outputFile, struct stat *stats)
{
    const char *linkName = get_file_name(filePath);

    struct stat target_stats;

    if (stat(filePath, &target_stats) == -1)
    {
        perror("Error getting target stats: ");
        exit(EXIT_FAILURE);
    }

    char user_rights[4], group_rights[4], others_rights[4];
    get_permissions(stats, user_rights, group_rights, others_rights);

    char buffer[1024];
    int buffer_length = sprintf(buffer, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier: %ld\ndrepturi de acces user legatura: %3s\ndrepturi de acces grup legatura: %3s\ndrepturi de acces altii legatura: %3s\n",
                                linkName,
                                (unsigned long)(*stats).st_size,
                                target_stats.st_size,
                                user_rights,
                                group_rights,
                                others_rights);

    write(outputFile, buffer, buffer_length);
}

void read_directory_files(const char *dirPath, const char *outputDir, const char *c)
{
    DIR *dir = opendir(dirPath);
    if (dir == NULL)
    {
        perror("Error opening directory: ");
        exit(EXIT_FAILURE);
    }

    int outputFile = -1;

    struct dirent *entry;

    printf("Processing directory files...\n\n");
    while ((entry = readdir(dir)) != NULL)
    {
        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        struct stat stats;
        get_file_lstats(filePath, &stats);

        // Ignore '.' and '..' entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // creez cele doua pipe-uri
        int pipe1[2], pipe2[2];
        if ((pipe(pipe1) < 0) || (pipe(pipe2) < 0))
        {
            perror("Error creating pipes: \n");
            exit(EXIT_FAILURE);
        }

        // Creez procesul fiu care scrie fisierele de statistica in functie de tipul fisierului
        int pid = fork();
        if (pid == -1)
        {
            perror("Error forking process: ");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            int total_lines_written = 0;
            char outputFileName[1024];
            snprintf(outputFileName, sizeof(outputFileName), "%s/%s_statistica.txt", outputDir, entry->d_name);

            outputFile = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
            // printf("outputFileName: %s\n", outputFileName);

            if (S_ISREG(stats.st_mode))
            {
                const char *ext = strrchr(entry->d_name, '.');
                if (ext != NULL && strcmp(ext, ".bmp") == 0)
                {
                    printf("Process bmp file...\n");
                    process_BMP_file(filePath, outputFile);

                    // Creez al doilea proces fiu care face conversia imaginilor bmp in imagini alb-negru
                    int grayscale_pid = fork();
                    if (grayscale_pid == -1)
                    {
                        perror("Error forking grayscale process: ");
                        exit(EXIT_FAILURE);
                    }
                    else if (grayscale_pid == 0)
                    {
                        convert_to_grayscale(filePath);
                        exit(EXIT_SUCCESS);
                    }
                }
                else
                {
                    printf("Processing regular file...\n");
                    write_reg_file(filePath, outputFile, &stats);
                    printf("Finished writing statistics for txt file.\n");

                    // Inchid ambele capete ale pipe-ului 2
                    close(pipe2[0]);
                    close(pipe2[1]);

                    // Inchid citirea pentru pipe-ul 1
                    close(pipe1[0]);
                    printf("test\n");

                    // Redirectez stdout sa scrie in pipe-ul 1(stdout are file descriptor = 1)
                    if (dup2(pipe1[1], 1) < 0)
                    {
                        perror("Error redirecting stdout: ");
                        exit(EXIT_FAILURE);
                    }

                    // Trimit continutul fisierului pe care il iau folosind comanada cat si il trimit pe pipe1
                    execlp("cat", "cat", filePath, NULL);

                    // Creez al doilea proces fiu care executa scriptul sh care face verificarea propozitiilor, dupa ce statistica a fost scrisa de primul fiu
                    int exec_script_pid = fork();
                    if (exec_script_pid == -1)
                    {
                        perror("Error forking execute script process: ");
                        exit(EXIT_FAILURE);
                    }
                    else if (exec_script_pid == 0)
                    {
                        // Inchid scrierea in pipe1
                        close(pipe1[1]);

                        // Redirectez stdin sa citeasca din pipe-ul 1(stdin are file descriptor = 0)
                        if (dup2(pipe1[0], 0) < 0)
                        {
                            perror("Error redirecting stdin: ");
                            exit(EXIT_FAILURE);
                        }

                        // Inchid citirea in pipe2
                        close(pipe2[0]);

                        if (dup2(pipe2[1], 1) < 0)
                        {
                            perror("Error redirecting stdout: ");
                            exit(EXIT_FAILURE);
                        }

                        execlp("/bin/sh", "/bin/sh", "./regex_s9.sh", c, NULL);
                    }

                    close(pipe1[0]);
                    close(pipe1[1]);

                    close(pipe2[1]);

                    // Citesc din pipe-ul 2
                    char buffer[20] = "";
                    if (read(pipe2[0], buffer, sizeof(buffer)) < 0)
                    {
                        perror("Error reading pipe2: ");
                        exit(EXIT_FAILURE);
                    }
                    int cnt = atoi(buffer);
                    printf("cnt = %d\n", cnt);
                    correct_sentences += cnt;
                    printf("valid_sentences = %d\n", correct_sentences);
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
                process_link(filePath, outputFile, &stats);
            }

            exit(EXIT_SUCCESS);
        }
        else if (pid > 0)
        {
            // Procesul parinte continua urmatoarea iteratie
            int status;
            waitpid(pid, &status, 0);
            printf("S-a încheiat procesul cu pid-ul %d și codul %d\n\n", pid, WEXITSTATUS(status));
        }
    }

    // close input directory
    closedir(dir);
    //close(outputFile);

    printf("Finished processing directory!\n");
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <input_directory> <output_directory> c<>\n", argv[0]);
        return 1;
    }

    char ch = argv[3][0];
    if (!isalnum(ch))
    {
        printf("Argumentul <c> trebuie sa fie un caracter alfanumeric!\n");
        return 1;
    }

    char *inputPath = argv[1];
    char *outputPath = argv[2];

    read_directory_files(inputPath, outputPath, &ch);
    printf("Propozitii valide: %d\n", correct_sentences);

    return 0;
}
