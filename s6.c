#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>

typedef struct bmp_header_t
{
    char file_name[100];
    int height;
    int width;
    int file_size;
    int user_id;
    char last_modified_date[20];
    int links;
    char user_rights[4];
    char group_rights[4];
    char others_rights[4];
} bmp_header_t;

int open_file(const char *pathname, int o_flags)
{
    int file_descriptor = open(pathname, o_flags);
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

void get_file_name(bmp_header_t *bmp_header, const char *file_name)
{
    strcpy(bmp_header->file_name, file_name);
}

void get_image_width(int file_descriptor, int *width)
{
    if (lseek(file_descriptor, 18, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if (read(file_descriptor, width, 4) < 0)
    {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
    }
}

void get_image_height(int file_descriptor, int *height)
{
    if (lseek(file_descriptor, 22, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if (read(file_descriptor, height, 4) < 0)
    {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
    }
}

void get_image_size(int file_descriptor, int *size)
{
    if (lseek(file_descriptor, 2, SEEK_SET) < 0)
    {
        perror("Error moving file cursor: ");
        exit(EXIT_FAILURE);
    }

    if (read(file_descriptor, size, 4) < 0)
    {
        perror("Error reading from file: ");
        exit(EXIT_FAILURE);
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

void get_user_id(struct stat *stats, bmp_header_t *bmp_header)
{
    bmp_header->user_id = (int)stats->st_uid;
}

void get_last_modified_date(struct stat *stats, bmp_header_t *bmp_header)
{
    strftime(bmp_header->last_modified_date, 20, "%d.%m.%Y", localtime(&stats->st_mtime));
}

void get_links(struct stat *stats, bmp_header_t *bmp_header)
{
    bmp_header->links = (int)stats->st_nlink;
}

void get_permissions(struct stat *stats, bmp_header_t *bmp_header)
{
    mode_t mode = stats->st_mode;

    // set permissions for user
    bmp_header->user_rights[0] = (mode & S_IRUSR) ? 'r' : '-';
    bmp_header->user_rights[1] = (mode & S_IWUSR) ? 'w' : '-';
    bmp_header->user_rights[2] = (mode & S_IXUSR) ? 'x' : '-';
    bmp_header->user_rights[3] = '\0';

    // set permissions for group
    bmp_header->group_rights[0] = (mode & S_IRGRP) ? 'r' : '-';
    bmp_header->group_rights[1] = (mode & S_IWGRP) ? 'w' : '-';
    bmp_header->group_rights[2] = (mode & S_IXGRP) ? 'x' : '-';
    bmp_header->group_rights[3] = '\0';

    // set permissions for others
    bmp_header->others_rights[0] = (mode & S_IROTH) ? 'r' : '-';
    bmp_header->others_rights[1] = (mode & S_IWOTH) ? 'w' : '-';
    bmp_header->others_rights[2] = (mode & S_IXOTH) ? 'x' : '-';
    bmp_header->others_rights[3] = '\0';
}

void fill_bmp_header(bmp_header_t *bmp_header, int image_descriptor, struct stat *stats)
{
    get_image_width(image_descriptor, &bmp_header->width);
    get_image_height(image_descriptor, &bmp_header->height);
    get_image_size(image_descriptor, &bmp_header->file_size);
    get_user_id(stats, bmp_header);
    get_links(stats, bmp_header);
    get_last_modified_date(stats, bmp_header);
    get_permissions(stats, bmp_header);
}

char *create_buffer(bmp_header_t bmp_header)
{
    char *buffer = (char *)malloc(200 * sizeof(char));
    sprintf(buffer, "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %d\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %10s\ncontorul de legaturi: %d\ndrepturi de acces user: %3s\ndrepturi de acces grup: %3s\ndrepturi de acces altii: %3s\n",
            bmp_header.file_name,
            bmp_header.height,
            bmp_header.width,
            bmp_header.file_size,
            bmp_header.user_id,
            bmp_header.last_modified_date,
            bmp_header.links,
            bmp_header.user_rights,
            bmp_header.group_rights,
            bmp_header.others_rights);
    return buffer;
}

void write_statistics_to_file(const char *file_name, const char *buffer)
{
    int file_descriptor = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (file_descriptor < 0)
    {
        perror("Can not open statistics file: ");
        exit(EXIT_FAILURE);
    }

    if (write(file_descriptor, buffer, strlen(buffer)) < 0)
    {
        perror("Can not write to statistics file: ");
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    close(file_descriptor);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    bmp_header_t bmp_header;
    struct stat stats;
    char inputFile[50];

    get_file_name(&bmp_header, argv[1]);
    strcpy(inputFile, bmp_header.file_name);

    // Deschide directorul de intrare
    process_dir_entries(inputFile);

    //set open flags
    int o_flags = O_RDONLY;
    int file_descriptor = open_file(bmp_header.file_name, o_flags);

    get_file_stats(bmp_header.file_name, &stats);

    fill_bmp_header(&bmp_header, file_descriptor, &stats);

    write_statistics_to_file("statistica.txt", create_buffer(bmp_header));

    close_file(file_descriptor);

    return 0;
}