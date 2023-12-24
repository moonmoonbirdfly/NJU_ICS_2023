#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

enum {FD_STDIN, FD_STDOUT, FD_STDERR, DEV_EVENTS, PROC_DISPINFO,FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read,  serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read,   serial_write},//invalid_write
  [DEV_EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write},
  [PROC_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
#include "files.h"
};
#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))
int fs_open(const char *pathname, int flags, int mode){
  for (int i = 0; i < NR_FILES; i++) {
        if (strcmp(file_table[i].name, pathname) == 0) {
            file_table[i].open_offset = 0;
            return i;
        }
    }
  panic("file %s not found", pathname);
  return -1;
}


int fs_close(int fd){
 file_table[fd].open_offset = 0;
  return 0;
}

size_t fs_read(int fd, void *buf, size_t len){
  ReadFn readFn = file_table[fd].read;
  if (readFn != NULL) {
        // 特殊文件处理
        size_t open_offset = file_table[fd].open_offset;
        return readFn(buf, open_offset, len);
  }
  size_t read_len = len;
  size_t open_offset = file_table[fd].open_offset;
  size_t size = file_table[fd].size;
  size_t disk_offset = file_table[fd].disk_offset;
  if (open_offset > size) return 0;
  if (open_offset + len > size) read_len = size - open_offset;
  ramdisk_read(buf, disk_offset + open_offset, read_len);
  file_table[fd].open_offset += read_len;
  return read_len;

}

size_t fs_write(int fd, const void *buf, size_t len) {
    // 首先，检查文件描述符是否有效，特别是如果 fd 是标准输入，则不能写入
    if (fd < 0 || fd >= NR_FILES) {
        Log("Invalid file descriptor: %d", fd);
        return 0;
    }

    // 获取关联的文件信息结构
    Finfo *finfo = &file_table[fd];

    // 特殊文件处理，比如写到串口或帧缓冲区
    if (finfo->write != NULL) {
        // 调用特定文件的写函数
        return finfo->write(buf, finfo->open_offset, len);
    }

    // 标准磁盘文件处理
    if (finfo->disk_offset != 0) {
        size_t write_capacity = finfo->size - finfo->open_offset;
        size_t to_write = len < write_capacity ? len : write_capacity;

        if (to_write > 0) {
            // 实际写入磁盘的函数
            size_t written = ramdisk_write(buf, finfo->disk_offset + finfo->open_offset, to_write);
            // 更新文件偏移量
            finfo->open_offset += written;
            return written;
        } 
        return 0; // 如果没有空间写入，则返回0
    }

    // 对于没有设置写操作的文件，返回无效操作
    Log("Write operation not supported for %s", finfo->name);
    return -1;
}


size_t fs_lseek(int fd, size_t offset, int whence){
  if (fd <= 2) {
        Log("ignore lseek %s", file_table[fd].name);
        return 0;
  }

  Finfo *file = &file_table[fd];
  size_t new_offset;
  // 根据 whence 参数来计算新的指针位置
    if (whence == SEEK_SET) {
        new_offset = offset;
    } else if (whence == SEEK_CUR) {
        new_offset = file->open_offset + offset;
    } else if (whence == SEEK_END) {
        new_offset = file->size + offset;
    } else {
        Log("Invalid whence value: %d", whence);
        return -1;
    }
     // 检查新的指针位置是否在文件范围内
    if (new_offset < 0 || new_offset > file->size) {
        Log("Seek position out of bounds");
        return -1;
    }
     // 设置新的文件读写指针
    file->open_offset = new_offset;
    
    return new_offset;
}

void init_fs() {
  // TODO: initialize the size of /0dev/fb
 AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  int height = ev.height;
   file_table[FD_FB].size = width * height * sizeof(uint32_t);
}
