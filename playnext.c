#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FNAME ".playnext"
#define PLAY "mpv --no-terminal "
const char* extensions[] = {
    ".mkv",
    ".mp4",
};

static int isepisode(const char* fname)
{
    int i;
    for(i = strlen(fname) - 1; i >= 0; i--)
        if(fname[i] == '.') break;
    if(i == -1) return 0;
    fname += i;
    for(int e = 0; e < sizeof(extensions) / sizeof(extensions[0]); e++)
        if(strcmp(fname, extensions[e]) == 0) return 1;
    return 0;
}

static int getn(void)
{
    FILE* f;
    int n, res;

    f = fopen(FNAME, "r");
    if(f == NULL)
        switch(errno) {
            case ENOENT:
                return -1;
            default:
                fprintf(stderr, "failed to open file. '%s'\n", strerror(errno));
                exit(1);
        }
    res = fscanf(f, "%d\n", &n);
    if(res == EOF || res < 1) return -1;
    return n;
}

static void setn(int n)
{
    FILE* f;
    char str[25];

    f = fopen(FNAME, "w");
    if(f == NULL) {
        fprintf(stderr, "failed to write file (1). '%s'\n", strerror(errno));
        exit(1);
    }
    sprintf(str, "%d\n", n);
    if(fwrite(str, strlen(str), 1, f) < 1) {
        fprintf(stderr, "failed to write file (2). '%s'\n", strerror(errno));
        exit(1);
    }
}

static char *pathcat(const char* a, const char* b)
{
    char *s;
    int n, i;
    s = malloc(strlen(a) + strlen(b) + 2);
    n = 0;
    i = 0;
    while(a[i]) s[n++] = a[i++];
    s[n++] = '/';
    i = 0;
    while(b[i]) s[n++] = b[i++];
    s[n] = '\0';
    return s;
}

static char *getepisode(const char *path, int *n)
{
    struct dirent **list;
    char *episode, *subPath;
    int count;

    count = scandir(path, &list, NULL, alphasort);
    if(count < 0) return NULL;
    for(int i = 2; i < count; i++) {
        if(list[i]->d_name[0] == '.') continue;
        if(list[i]->d_type == DT_DIR) {
            subPath = pathcat(path, list[i]->d_name);
            episode = getepisode(subPath, n);
            free(subPath);
            if(episode != NULL) return episode;
        } else {
            if(!isepisode(list[i]->d_name)) continue;
            if(*n == 0) return pathcat(path, list[i]->d_name);
            (*n)--;
        }
    }
    free(list);
    return NULL;
}

static void play(char* f)
{
    char* args[] = {"mpv", "--no-terminal", f, NULL};
    char* cmd = malloc(strlen(f) + strlen(PLAY) + 3);
    strcpy(cmd, PLAY);
    strcat(cmd, "\"");
    strcat(cmd, f);
    strcat(cmd, "\"");
    printf("%s\n", cmd);

    if(system(cmd) != 0)
        fprintf(stderr, "failed to play episode '%s'\n", strerror(errno));
}

int main(int argc, char *argv[])
{
    int n, checked, next = 1;
    char *episode;

    if(argc == 2 && strcmp(argv[1], "-r") == 0) next = 0;

    n = getn() + next;
    checked = n;
    episode = getepisode(".", &checked);
    if(episode) {
        printf("PLAYING %s\n", episode);
        play(episode);
        free(episode);
    } else {
        printf("FINISHED\n");
    }
    setn(n);
}
