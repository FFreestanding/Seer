#include <filesystem/ext2.h>
#include <kernel_io/vga.h>
#include <kernel_io/memory.h>
#include "kernel_io/valloc.h"
#include "device/sata/ahci.h"
#include "apic/keyboard.h"
#include "data_structure/kernel_string.h"
#include "apic/timer.h"
#include "paging/vmm.h"
#include "paging/page.h"
#include "data_structure/kernel_bitset.h"

static struct ext2_context ext2_ctx;


void logh(char *info, char *buff, uint32_t len)
{
    kernel_log(INFO, "----------[%s]----------", info);
    for (uint32_t i = 0; i < len; i++)
    {
        kprintf("%h ", buff[i]);
    }
    kprintf("\n");
    kernel_log(INFO, "------------------------");
}

static void read_cmd(char *buff)
{
    char *path = strchr(buff, " ");
    if (path==NULL_POINTER){ return; }
    else { path+=1; }

    char *path_cpy = valloc(strlen(path));
    memory_copy(path, path_cpy, strlen(path));

    struct ext2_inode *inode = ext2_inode_get(path);
    if (inode == NULL_POINTER) {
        kernel_log(WARN, "No Such File or Directory");
        goto done;
    } else if ((inode->i_mode & EXT2_S_IFREG) == 0) {
        kernel_log(WARN, "It is not a File");
        goto done;
    }

    uint8_t *file_data_buffer = valloc(ext2_ctx.block_size);
    for (uint32_t i = 0; i < 12; ++i) {
        if(inode->i_block[i]==0){break;}
        uint32_t block_offset = inode->i_block[i] * ext2_ctx.block_size / PHYSICAL_BLOCK_SIZE;
        ext2_read_data(file_data_buffer, block_offset, PHYSICAL_BLOCK_COUNTS(ext2_ctx.block_size));
        kernel_log(INFO, "%s", file_data_buffer);
    }

done:
    vfree(file_data_buffer);
    vfree(inode);
//    vfree(dir_entry_vfree_p);
    vfree(path_cpy);
}

static void write_cmd(char *buff)
{
    char *path = strchr(buff, " ");
    if (path==NULL_POINTER){ return; }
    else { path+=1; }

    char *path_cpy = valloc(strlen(path));
    memory_copy(path, path_cpy, strlen(path));

    // contents -> "hello ext2"
    char *contents = strchr(path, "\"");
    if (contents==NULL_POINTER){ return; }
    else { contents+=1; }
    for (int i = 0; i < strlen(contents); ++i) {
        if (*(contents+i) == '"')
        {
            *(contents+i) = '\0';
        }
    }

    struct ext2_inode *inode = ext2_inode_get(path);
    if (strlen(contents)>=ext2_ctx.block_size) {
        goto done;
    }
    if (inode == NULL_POINTER) {
        kernel_log(WARN, "No Such File or Directory");
        goto done;
    } else if ((inode->i_mode & EXT2_S_IFREG) == 0) {
        kernel_log(WARN, "It is not a File");
        goto done;
    }

    uint8_t *file_data_buffer = valloc(ext2_ctx.block_size);
    memory_copy(contents, file_data_buffer, strlen(contents));

    ext2_write_data(file_data_buffer, inode->i_block[0] * ext2_ctx.block_size / PHYSICAL_BLOCK_SIZE,
                    1);

    vfree(file_data_buffer);
done:
    vfree(inode);
    vfree(path_cpy);
}

static void mkdir_cmd(char *buff)
{
    char *path = strchr(buff, " ");
    if (path==NULL_POINTER){ return; }
    else { path+=1; }

    char *path_cpy = valloc(strlen(path));
    memory_copy(path, path_cpy, strlen(path));

    char **sub_str = strsplit(path, FS_PATH_DELIMITER);
    uint32_t sub_str_count = 0;
    for (uint32_t i = 0; i < 16; ++i) {
        if (sub_str[i]!=0) {
            ++sub_str_count;
        }
    }

    if (sub_str_count < 1) {
        return;
    }

    char *mkdir_name = valloc(strlen(sub_str[sub_str_count-1]));
    memory_copy(sub_str[sub_str_count-1], mkdir_name,
                strlen(sub_str[sub_str_count-1]));

    char *p = path_cpy + strlen(path_cpy) - 1;
    for (int i = strlen(sub_str[sub_str_count-1]); i >= 0; --i) {
        *p = 0;
        --p;
    }
    struct ext2_inode *dir_inode = ext2_inode_get(path_cpy);

    if (dir_inode == NULL_POINTER) {
        kernel_log(WARN, "No Such A Directory");
        goto done;
    }
    if ((dir_inode->i_mode & EXT2_S_IFDIR) == 0) {
        kernel_log(INFO, "Parent path is not a Directory path", path);
        goto done;
    }

    struct ext2_directory_entry *dir_entry = valloc(ext2_ctx.block_size);
    struct ext2_directory_entry *dir_entry_vfree_p = dir_entry;
    uint32_t dir_entry_disk_block_offset = 0;
    struct ext2_directory_entry *dir_entry_p;
    uint32_t recent_inode = 0;
    uint32_t dir_offet_cpy = 0;
    for (uint32_t block_number = 0; block_number < 12 && dir_inode->i_block[block_number]; ++block_number) {
        dir_entry_disk_block_offset = dir_inode->i_block[block_number] * ext2_ctx.block_size / PHYSICAL_BLOCK_SIZE;
        ext2_read_data(dir_entry, dir_entry_disk_block_offset,
                       PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_directory_entry)));
        dir_entry_p = dir_entry;
        dir_offet_cpy = dir_entry_disk_block_offset;
        while (dir_entry->rec_len>0)
        {
            recent_inode = dir_entry->inode;
            if (0==strncmp(mkdir_name, dir_entry->name, dir_entry->rec_len)) {
                kernel_log(INFO, "Directory already exists");
                goto done;
            }
//            if (dir_entry->rec_len != sizeof(struct ext2_directory_entry)+dir_entry->name_len)
//            {
//                dir_entry->rec_len
//            }
            dir_entry = (struct ext2_directory_entry *)
                    ((uint8_t *)dir_entry + sizeof(struct ext2_directory_entry) + dir_entry->name_len);
        }
    }

    uint32_t recent_group = find_block_group_index(recent_inode);
    uint32_t block_bitmap_offset = find_block_bitmap_block_offset(recent_group);
    uint32_t inode_bitmap_offset = find_inode_bitmap_block_offset(recent_group);
    uint8_t *block_bitmap = valloc(ext2_ctx.block_size);
    uint8_t *inode_bitmap = valloc(ext2_ctx.block_size);
    ext2_read_data(block_bitmap, block_bitmap_offset,
                   PHYSICAL_BLOCK_COUNTS(ext2_ctx.block_size));
    ext2_read_data(inode_bitmap, inode_bitmap_offset,
                   PHYSICAL_BLOCK_COUNTS(ext2_ctx.block_size));
    kbitset bk = kernel_bitset_create(block_bitmap, ext2_ctx.block_size*8);
    kbitset ik = kernel_bitset_create(inode_bitmap, ext2_ctx.block_size*8);
    uint32_t free_block_index = find_unset_bit_index(&bk);
    uint32_t free_inode_index = find_unset_bit_index(&ik);

    uint32_t first_block_offset = find_first_block_block_offset(recent_group);
    dir_entry->name_len = strlen(mkdir_name);
    dir_entry->rec_len = sizeof(struct ext2_directory_entry) + dir_entry->name_len;
    memory_copy(mkdir_name, dir_entry->name, dir_entry->name_len);
    dir_entry->inode = free_inode_index;
    ext2_write_data(dir_entry_p, dir_offet_cpy,
                   PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_directory_entry)));

    struct ext2_inode *inode_table = valloc(ext2_ctx.block_size);

    uint32_t offset1 = find_inode_table_block_offset(recent_group);
    uint32_t local_index = find_local_inode_index(dir_entry->inode);
    uint32_t bytes_size = offset1*ext2_ctx.block_size + local_index*ext2_ctx.s_inode_size;
    uint32_t left_size = bytes_size % PHYSICAL_BLOCK_SIZE;
    ext2_read_data(inode_table,
                   bytes_size/ext2_ctx.block_size*(ext2_ctx.block_size/PHYSICAL_BLOCK_SIZE),
                   PHYSICAL_BLOCK_COUNTS(ext2_ctx.block_size));

    struct ext2_inode *modified_inode = (uint8_t *)inode_table + left_size;
    modified_inode->i_block[0] = free_block_index + recent_group * ext2_ctx.block_size * 8;
    modified_inode->i_mode = EXT2_S_IFDIR;

    ext2_write_data(inode_table, find_inode_table_block_offset(recent_group) / PHYSICAL_BLOCK_SIZE, PHYSICAL_BLOCK_COUNTS(ext2_ctx.block_size));

    vfree(mkdir_name);
    vfree(dir_inode);
    vfree(dir_entry_vfree_p);
    vfree(inode_bitmap);
    vfree(block_bitmap);
done:
    vfree(path_cpy);

}

static void cd_cmd(char *buff)
{
    kernel_log(INFO, "cd command");
}

static void touch_cmd(char *buff)
{
    kernel_log(INFO, "touch command");
}

static void list_cmd(char *buff)
{
    char *path = strchr(buff, " ");
    if (path==NULL_POINTER){ return; }
    else { path+=1; }

    char *path_cpy = valloc(strlen(path));
    memory_copy(path, path_cpy, strlen(path));

    struct ext2_inode *inode = ext2_inode_get(path);
    if (inode == NULL_POINTER) {
        kernel_log(WARN, "No Such File or Directory");
        goto done;
    }
    if (inode->i_mode & EXT2_S_IFREG) {
        kernel_log(INFO, "file: %s", path);
    } else if (inode->i_mode & EXT2_S_IFDIR) {
        kernel_log(INFO, "directory: %s", path);
    } else {
        kernel_log(INFO, "unknown type: %s", path);
    }

    struct ext2_directory_entry *dir_entry = VALLOC_DIR_ENTRY();
    struct ext2_directory_entry *dir_entry_vfree_p = dir_entry;
    uint32_t dir_entry_disk_block_offset = 0;
    uint32_t dir_entry_byte_offset = 0;
    int total_size = (int) ext2_ctx.block_size;

    char *n = valloc(32);

    for (int block_number = 0; block_number < 12 && inode->i_block[block_number]; ++block_number) {
        dir_entry_disk_block_offset = inode->i_block[block_number] * ext2_ctx.block_size / PHYSICAL_BLOCK_SIZE;
        ext2_read_data(dir_entry, dir_entry_disk_block_offset,
                       PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_directory_entry)));
        while (total_size)
        {
            memory_set(n, 0, 32);
            memory_copy(dir_entry->name, n, dir_entry->name_len);
            kernel_log(INFO, "%s", n);
            total_size -= dir_entry->rec_len;
            dir_entry_byte_offset += dir_entry->rec_len;
            if (dir_entry_byte_offset >= PHYSICAL_BLOCK_SIZE) {
                ++dir_entry_disk_block_offset;
                dir_entry_byte_offset -= PHYSICAL_BLOCK_SIZE;
                ext2_read_data(inode, dir_entry_disk_block_offset,
                               PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_directory_entry)));
            }
            dir_entry = (struct ext2_directory_entry *)((uint8_t *)dir_entry + dir_entry->rec_len);
        }
    }

    vfree(n);
    vfree(inode);
    vfree(dir_entry_vfree_p);
done:
    vfree(path_cpy);
}

static char buff[KEY_BUFFER_SIZE];
static uint32_t buff_write_p = 0;
void
read_keyboard_buff(void *mgr)
{
    uint32_t n = read_keyboard_buffer_manager(mgr, buff+buff_write_p, 1);
    if (n==0) { return; }

    buff_write_p += n;
    if (buff_write_p >= KEY_BUFFER_SIZE) {
        buff_write_p = 0;
        memory_set_fast(buff, 0, KEY_BUFFER_SIZE/4);
    }

    char *find_newline = NULL;
    char *op = NULL;

    find_newline = strstr(buff, "\n");
    if (find_newline == NULL) { return; }
    *find_newline = '\0';

    op = strstr(buff, "read");
    if (op != NULL && op == buff) { read_cmd(buff); goto clear_buffer; }

    op = strstr(buff, "write");
    if (op != NULL && op == buff) { write_cmd(buff); goto clear_buffer; }

    op = strstr(buff, "mkdir");
    if (op != NULL && op == buff) { mkdir_cmd(buff); goto clear_buffer; }

    op = strstr(buff, "cd");
    if (op != NULL && op == buff) { cd_cmd(buff); goto clear_buffer; }

    op = strstr(buff, "touch");
    if (op != NULL && op == buff) { touch_cmd(buff); goto clear_buffer; }

    op = strstr(buff, "list");
    if (op != NULL && op == buff) { list_cmd(buff); goto clear_buffer; }

    kernel_log(WARN, "Command Not Found");
clear_buffer:
    memory_set_fast(buff, 0, KEY_BUFFER_SIZE/4);
    buff_write_p = 0;
    kprintf("[%s] ", ext2_ctx.current_path);
    tty_sync_cursor();
}

void ext2_app()
{
    static struct ext2_superblock *super;
    struct ext2_block_group_descriptor *desc;
    super = valloc(sizeof(struct ext2_superblock));
    desc = VALLOC_DESCRIPTOR();

    ext2_read_super_block(super);
    uint32_t desc_offset = 0;
    if ((1024 << super->s_log_block_size) > 1024*2) {
        desc_offset = (1024 << super->s_log_block_size);
    } else {
        desc_offset = 1024*2;
    }
    ext2_read_data(desc, desc_offset/PHYSICAL_BLOCK_SIZE,
                   PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_block_group_descriptor)));

    init_ext2_context(super, desc);
    vfree(super);
    vfree(desc);

    kernel_log(INFO, "\n=== Welcome to EXT2 File System ===");
    kernel_log(INFO, "     Only Support Absolute Path     ");
    kernel_log(INFO, "example: read /Seer/text.txt");
    kernel_log(INFO, "         write /Seer/text.txt 'hello'");
    kernel_log(INFO, "         mkdir /Seer/test_dir/");
    kernel_log(INFO, "         cd /Seer/text/");
    kernel_log(INFO, "         touch /Seer/test.md");
    kernel_log(INFO, "         list /Seer/");
//    kernel_log(INFO, "         rm test.md");
//    kernel_log(INFO, "         rmdir /root/text");
    kernel_log(INFO, "===================================");
//    mkdir_cmd("mkdir /eee/");

    kprintf("[%s] ", ext2_ctx.current_path);
    tty_sync_cursor();
    struct keyboard_buffer_manager *mgr = get_keyboard_buffer_manager();
    timer_run_ms(1, read_keyboard_buff, mgr, 1);
    while (1);
}

void init_ext2_context(struct ext2_superblock *super_block, struct ext2_block_group_descriptor *desc)
{
    memory_set(ext2_ctx.current_path, 0, sizeof(struct ext2_context));
    ext2_ctx.block_size = 1024 << super_block->s_log_block_size;
    ext2_ctx.s_inodes_per_group = super_block->s_inodes_per_group;
//    ext2_ctx.block_of_first_group = super_block->s_first_data_block;
    ext2_ctx.s_blocks_per_group = super_block->s_blocks_per_group;
    ext2_ctx.s_inode_size = super_block->s_inode_size;
    ext2_ctx.group_counts = super_block->s_blocks_count / super_block->s_blocks_per_group;
    uint32_t left = super_block->s_blocks_count % super_block->s_blocks_per_group;
    if (2*left >= super_block->s_blocks_per_group)
    {
        ++ext2_ctx.group_counts;
    }
    ext2_ctx.current_path[0] = *(char *) FS_PATH_DELIMITER;

    ext2_ctx.root_inode_table = desc->bg_inode_table;
    ext2_ctx.bg_used_dirs_count = desc->bg_used_dirs_count;

    ASSERT(ext2_ctx.block_size>=1024, "init_ext2_context");
}

void ext2_read_data(uint8_t *buffer, uint32_t block_offset, uint32_t counts)
{
    struct hba_port* port = ahci_get_port(0);
    int result = 0;

    for (uint32_t i = 0; i < counts; ++i) {
        result = port->device->ops.read_buffer(port, block_offset + i,
                                               buffer + (i*512), port->device->block_size);
        ASSERT(result!=0, "ext2_read_data");
    }
}

void ext2_write_data(uint8_t *buffer, uint32_t block_offset, uint32_t counts)
{
    struct hba_port* port = ahci_get_port(0);
    int result = 0;

    for (uint32_t i = 0; i < counts; ++i) {
        result = port->device->ops.write_buffer(port, block_offset + i,
                                               buffer + (i*512), port->device->block_size);
        ASSERT(result!=0, "ext2_write_data");
    }
}

void ext2_read_super_block(uint8_t *buffer)
{
    ext2_read_data(buffer, 1024/PHYSICAL_BLOCK_SIZE, 1024/PHYSICAL_BLOCK_SIZE);//1024 bytes
}

struct ext2_inode *ext2_inode_get(char *path)
{
    struct ext2_inode *inode_table = valloc(ext2_ctx.block_size);
    ext2_read_data(inode_table, ext2_ctx.root_inode_table * ext2_ctx.block_size / PHYSICAL_BLOCK_SIZE,
                   PHYSICAL_BLOCK_COUNTS(ext2_ctx.block_size));

    struct ext2_inode *current_inode = &inode_table[EXT2_ROOT_INO];

    char **sub_path = strsplit(path, FS_PATH_DELIMITER);
    uint32_t sub_path_i = 0;

    struct ext2_directory_entry *dir_entry = VALLOC_DIR_ENTRY();
    struct ext2_directory_entry *dir_entry_vfree_p = dir_entry;
    uint32_t total_dir_entries_bytes = ext2_ctx.block_size;

    for (int i = 0; sub_path[sub_path_i] != NULL_POINTER && i < 12; ++i) {
        total_dir_entries_bytes = ext2_ctx.block_size;
        ext2_read_data(dir_entry, current_inode->i_block[i] * ext2_ctx.block_size / PHYSICAL_BLOCK_SIZE,
                       PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_directory_entry)));
        while (total_dir_entries_bytes)
        {
            if (0 == strncmp((const char *) sub_path[sub_path_i], dir_entry->name, dir_entry->name_len))
            {
                uint32_t block_group_index = find_block_group_index(dir_entry->inode);
                uint32_t local_inode_index = find_local_inode_index(dir_entry->inode);
                uint32_t offset1 = find_inode_table_block_offset(block_group_index);

                uint32_t bytes_size = offset1*ext2_ctx.block_size+local_inode_index*ext2_ctx.s_inode_size;
                uint32_t left_bytes_size = bytes_size % ext2_ctx.block_size;
                ext2_read_data(inode_table,
                               bytes_size/ext2_ctx.block_size*(ext2_ctx.block_size/PHYSICAL_BLOCK_SIZE),
                               PHYSICAL_BLOCK_COUNTS(ext2_ctx.block_size));

                current_inode = (uint8_t *)inode_table + left_bytes_size;
                i = -1;
                ++sub_path_i;
                break;
            }
            total_dir_entries_bytes -= dir_entry->rec_len;
            dir_entry = (struct ext2_directory_entry *)((uint8_t *)dir_entry + dir_entry->rec_len);
        }
    }

    struct ext2_inode *ret = NULL_POINTER;
    if (total_dir_entries_bytes==0){ goto done; }

    ret = VALLOC_INODE();
    memory_copy(current_inode, (uint8_t *) ret,
                PHYSICAL_BLOCK_SIZE * PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_inode)));

done:
    vfree(dir_entry_vfree_p);
    vfree(inode_table);
    vfree(sub_path);

    return ret;
}

uint32_t find_block_group_index(uint32_t inode)
{
    return (inode - 1) / ext2_ctx.s_inodes_per_group;
}

uint32_t find_local_inode_index(uint32_t inode)
{
    return (inode - 1) % ext2_ctx.s_inodes_per_group;
}

uint32_t find_inode_table_block_offset(uint32_t group_index)
{
//    block groups 0, 1 and powers of 3, 5 and 7.
    ASSERT(group_index<=7, "group_index is too big");
    uint32_t offset = 0;
    if (group_index==0 || group_index==1 || group_index==3 || group_index==5 || group_index==7)
    {
        struct ext2_block_group_descriptor *desc = VALLOC_DESCRIPTOR();
        ext2_read_data(desc, (ext2_ctx.block_size * (ext2_ctx.s_blocks_per_group * group_index +1 )) / PHYSICAL_BLOCK_SIZE,
                       PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_block_group_descriptor)));
        offset = desc->bg_inode_table;
        vfree(desc);
        return offset;
    } else {
        offset = 2;
        return ext2_ctx.s_blocks_per_group * group_index + offset;
    }
}

uint32_t find_block_bitmap_block_offset(uint32_t group_index)
{
    ASSERT(group_index<=7, "group_index is too big");

    if (group_index==0 || group_index==1 || group_index==3 || group_index==5 || group_index==7)
    {
        struct ext2_block_group_descriptor *desc = VALLOC_DESCRIPTOR();
        ext2_read_data(desc, (ext2_ctx.block_size * (ext2_ctx.s_blocks_per_group * group_index + 1)) / PHYSICAL_BLOCK_SIZE,
                       PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_block_group_descriptor)));
        uint32_t offset = desc->bg_block_bitmap;
        vfree(desc);
        return offset;
    } else {
        return ext2_ctx.s_blocks_per_group * group_index;
    }
}

uint32_t find_inode_bitmap_block_offset(uint32_t group_index)
{
    ASSERT(group_index<=7, "group_index is too big");

    if (group_index==0 || group_index==1 || group_index==3 || group_index==5 || group_index==7)
    {
        struct ext2_block_group_descriptor *desc = VALLOC_DESCRIPTOR();
        ext2_read_data(desc, (ext2_ctx.block_size * (ext2_ctx.s_blocks_per_group * group_index+1)) / PHYSICAL_BLOCK_SIZE,
                       PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_block_group_descriptor)));
        uint32_t offset = desc->bg_inode_bitmap;
        vfree(desc);
        return offset;
    } else {
        return ext2_ctx.s_blocks_per_group * group_index + 1;
    }
}

uint32_t find_first_block_block_offset(uint32_t group_index)
{
    return find_inode_table_block_offset(group_index)+1;
}
