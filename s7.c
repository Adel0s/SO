#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

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

void process_BMP_file(const char *filePath, int outputFile, bmp_header_t bmp_header)
{
    int fd_i = open(filePath, O_RDONLY);
    if (fd_i == -1)
    {
        perror("Error opening input file: \n");
        exit(EXIT_FAILURE);
    }
    if (read(fd_i, &bmp_header, sizeof(bmp_header_t)) != sizeof(bmp_header_t))
    {
        perror("Error reading: \n");
        exit(EXIT_FAILURE);
    }
    int width = bmp_header.width;
    int height = bmp_header.height;

    // Find the last occurrence of '/'
    const char *lastSlash = strrchr(filePath, '/');

    // Extract the file name
    const char *fileName = (lastSlash != NULL) ? lastSlash + 1 : filePath;

    struct stat stats;
    if (fstat(fd_i, &stats) == -1)
    {
        perror("Error getting file stats: ");
        close(fd_i);
        exit(EXIT_FAILURE);
    }

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

    printf("Writing into statistica.txt file...\n");
    write(outputFile, buffer, buffer_length);
    close(fd_i);
    close_file(outputFile);
    printf("Finished writing into statistica.txt file!\n");
}

void process_file(const char *filePath, int outputFile)
{
    int fd_i = open(filePath, O_RDONLY);

    // Find the last occurrence of '/'
    const char *lastSlash = strrchr(filePath, '/');
    // Extract the file name
    const char *fileName = (lastSlash != NULL) ? lastSlash + 1 : filePath;

    struct stat stats;
    if (fstat(fd_i, &stats) == -1)
    {
        perror("Eroare la obtinerea info file ");
        close(fd_i);
        exit(-1);
    }
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
    close_file(fd_i);
    close_file(outputFile);
}

void process_directory(const char *path, int outputFile)
{
    
}

void process_link(const char *filePath, int outputFile)
{

}

void read_directory_files(const char *dirPath, int outputFile)
{
    DIR *dir = opendir(dirPath);
    if (dir == NULL)
    {
        perror("Error opening directory: \n");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    printf("Processing directory files...\n");

    while ((entry = readdir(dir)) != NULL)
    {
        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

        struct stat stats;
        get_file_stats(filePath, &stats);

        // Ignoră intrările curente și părinte (valabil in cazul in care avem director in interiorul directorului dat ca input)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (S_ISREG(stats.st_mode))
        {
            const char *ext = strrchr(entry->d_name, '.');
	        if (ext != NULL && strcmp(ext, ".bmp") == 0)
            {
                printf("Process bmp file...\n");
                process_BMP_file(filePath, outputFile, bmp_header);
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
            printf("Processing symbolinc link...\n");
            process_link(filePath, outputFile);
        }
    }
    printf("Finished processing directory!\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <input_directory>\n", argv[0]);
        return 1;
    }

    char *inputPath = argv[1];
    int o_flags = O_WRONLY | O_CREAT | O_TRUNC;
    int o_mode = S_IRUSR | S_IWUSR;

    int outputFile = open_file("statistica.txt", o_flags, o_mode);

    read_directory_files(inputPath, outputFile);

    return 0;
}