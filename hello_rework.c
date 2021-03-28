#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <cjson/cJSON.h>
#include <stdlib.h>

cJSON *js;
cJSON *root;
cJSON *cur_js;



void init(){
    char * buffer = 0;
    long length;
    root = NULL;
    js = NULL;
    FILE * f = fopen ("example.json", "r");

    if (f)
    {
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      fseek (f, 0, SEEK_SET);
      buffer = malloc (length);
      if (buffer)
      {
        fread (buffer, 1, length, f);
      }
      fclose (f);
    }
    
    root = cJSON_Parse(buffer);
    js = root->child;
    free(buffer);
}

int find_the_js(const char *path){
    int i=0;
    int start = 0;
    while(1){
        char ar[100];
        int j = 0;
        for(;i<strlen(path); i++){
            if(path[i] == '/') {
                i++;
                break;
            }
            ar[j] = path[i];
            j++;
        }
        ar[j] = '\0';
        cJSON *temp = cur_js;
        while(strcmp(temp->string, ar) != 0){
            if(temp->next) temp = temp->next;
            else return 0;
        }
        if(temp->child) cur_js = temp->child;
        else return 0;
        if(i == strlen(path)) break;
    }
    return strlen(path);
}

int find_the_js2(const char *path){
    int i=0;
    int start = 0;
    while(1){
        char ar[100];
        int j = 0;
        for(;i<strlen(path); i++){
            if(path[i] == '/') {
                i++;
                break;
            }
            ar[j] = path[i];
            j++;
        }
        ar[j] = '\0';
        cJSON *temp = cur_js;
        while(strcmp(temp->string, ar) != 0){
            if(temp->next) temp = temp->next;
            else return 0;
        }
        cur_js = temp;
        if(i == strlen(path)) break;
        if(temp->child) cur_js = temp->child;
        else return 0;
    }
    return strlen(path);
}


void change_js(char * buffer){
    FILE * f = fopen ("example.json", "w");
    if(f) {
      if(buffer) fputs(buffer, f);
      fclose(f);
    }
    
}

static int hello_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    init();
    cur_js = js;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0644;
        stbuf->st_nlink = 2;
    }
    else {
        int len = find_the_js2(path + 1);
        if(len > 0) {
            if(cur_js->valuestring){
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = len+1;
            }
            else {
                stbuf->st_mode = S_IFDIR | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = len+1;
            }
        }
        else res = -ENOENT;
    }
    return res;
}

static int hello_unlink(const char *path)
{
    int i;
    char path2[100];
    cJSON* parent;
    strcpy(path2, path);
    cur_js = js;
    if(strlen(path) == 1) parent = js;
    else {
        for(i=strlen(path)-2; i>=0; i--){
            path2[i] = '\0';
            if(path[i] == '/') break;
        }
        int len = find_the_js2(path2 + 1);
        parent = cur_js;
    }
    const char *path_plus = path + 1;
    cur_js = js;
    int len = find_the_js2(path_plus);
    
    if(!cur_js->valuestring) return -EROFS;
    
    cJSON_DetachItemViaPointer(parent, cur_js);
    char *buffer = cJSON_Print(root);
    change_js(buffer);
    (void)path;

    init();
    return 0;
}

static int hello_rmdir(const char *path)
{
    int i;
    char path2[100];
    cJSON* parent;
    cur_js = js;
    strcpy(path2, path);
    if(strlen(path) == 1) parent = js;
    else {
        for(i=strlen(path)-2; i>=0; i--){
            path2[i] = '\0';
            if(path[i] == '/') break;
        }
        int len = find_the_js2(path2 + 1);
        parent = cur_js;
    }

    const char *path_plus = path + 1;
    cur_js = js;
    int len = find_the_js2(path_plus);
    
    if(cur_js->valuestring) return -EROFS;

    cJSON_DetachItemViaPointer(parent, cur_js);
    char *buffer = cJSON_Print(root);
    change_js(buffer);
    (void)path;

    init();
    return 0;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
    
    //if (strcmp(path, "/") != 0)
        //return -ENOENT;
    
    init();
    cur_js = js;
    int n = 2;
    if (strcmp(path, "/") != 0) n = find_the_js(path+1);
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    if(n < 1) return 0;
    filler(buf, cur_js->string, NULL, 0);
    while(cur_js->next){
        cur_js = cur_js->next;
        filler(buf, cur_js->string, NULL, 0);
    }

    return 0;
}


static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    const char *path_plus = path + 1;
    int n = 2;
    cur_js = js;
    if(strlen(path) != 1) n = find_the_js2(path_plus);

    if(n == 0) return -ENOENT;

    len = strlen(cur_js->valuestring);
    
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, cur_js->valuestring + offset, size);
    } else
        size = 0;
    return size;
}

static struct fuse_operations hello_oper = {
    .getattr	= hello_getattr,
    .unlink = hello_unlink,
    .rmdir = hello_rmdir,
    .readdir	= hello_readdir,
    .read	= hello_read
};


int main(int argc, char *argv[])
{
    init();

    return fuse_main(argc, argv, &hello_oper, NULL);
}
