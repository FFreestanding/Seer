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

static struct ext2_context ext2_ctx;
static struct ext2_superblock *super;
struct ext2_block_group_descriptor *desc_table;

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

    kernel_log(INFO, "read command");
}

static void write_cmd(char *buff)
{
    kernel_log(INFO, "write command");
}

static void mkdir_cmd(char *buff)
{
    kernel_log(INFO, "mkdir command");
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
    kernel_log(INFO, "list command");
    ext2_read_block_group_descriptor(desc_table);
    kernel_log(INFO, "1");
    uint32_t inode_table_bytes_offset = ext2_ctx.block_size * desc_table->bg_inode_table;
    kernel_log(INFO, "1 %h", inode_table_bytes_offset);
    struct ext2_inode *current_inode = vmm_alloc_page_entry(DEFAULT_PAGE_FLAGS, DEFAULT_PAGE_FLAGS);
    kernel_log(INFO, "1 %h", inode_table_bytes_offset/512);
    ext2_read_data(current_inode, inode_table_bytes_offset/512, 1);
    kernel_log(INFO, "2");
    struct ext2_inode *root_dir_inode = &current_inode[1];

    struct ext2_directory_entry *entry = valloc(512);
    for (int i = 0; i < 12; ++i) {
        ext2_read_data(entry, BLOCK_COUNTS(root_dir_inode->i_block[i]*ext2_ctx.block_size)/512,1);
        kernel_log(INFO, "name: %s", entry->name);
    }

}

//void parse_superblock(struct ext2_superblock *super_block_ptr)
//{
//
//    superblocks = *super_block_ptr;
//
//    if (superblocks.s_magic == EXT2_SUPER_MAGIC)
//    {
//        kernel_log(INFO, "It is ext2 filesystem. Magic Number: 0x%h\n",
//            superblocks.s_magic);
//    }
//    else
//    {
//        kernel_log(INFO, "[WARNNING] It is not ext2 filesystem. Magic Number: 0x%h\n",
//            superblocks.s_magic);
//    }
//
//    kernel_log(INFO, "s_volume_name:%s\n"
//            "s_last_mounted:%s\n"
//            "s_first_data_block:%h\n"
//            "block_size:%uB\n"
//            "s_feature_ro_compat:%h\n",
//            superblocks.s_volume_name,
//            superblocks.s_last_mounted,
//            superblocks.s_first_data_block,
//            (1024 << superblocks.s_log_block_size),
//            superblocks.s_feature_ro_compat);
//}

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
    kprintf("[/] ");
    tty_sync_cursor();
}

void ext2_app()
{
    super = valloc(sizeof(struct ext2_superblock));
    ext2_read_super_block(super);
    init_ext2_context(super);
    desc_table = valloc(super->s_block_group_nr*sizeof(struct ext2_block_group_descriptor));
    kernel_log(INFO, "\n=== Welcome to EXT2 File System ===");
    kernel_log(INFO, "example: read /root/text.txt");
    kernel_log(INFO, "         write /root/text.txt 'hello' ");
    kernel_log(INFO, "         mkdir /root/test_dir");
    kernel_log(INFO, "         cd /root/text");
    kernel_log(INFO, "         touch test.md");
    kernel_log(INFO, "         list");
//    kernel_log(INFO, "         rm test.md");
//    kernel_log(INFO, "         rmdir /root/text");
    kernel_log(INFO, "===================================");

    struct keyboard_buffer_manager *mgr = get_keyboard_buffer_manager();
    kprintf("[%s] ", ext2_ctx.current_path);
    tty_sync_cursor();
    timer_run_ms(1, read_keyboard_buff, mgr, 1);
    while (1);
}

void
parse_ext2img_tmp()
{
    struct hba_port* port = ahci_get_port(0);

    char *buffer = valloc(512);
    memory_set(buffer, '3', 512);
    int result;

    // 写入第一扇区 (LBA=0)
    result =
            port->device->ops.write_buffer(port, 0, buffer, port->device->block_size);
    if (!result) {
        kprintf("fail to write: %h\n", port->device->last_error);
    }

    memory_set(buffer, '6', port->device->block_size);

    // 读出我们刚刚写的内容！
    result =
            port->device->ops.read_buffer(port, 0, buffer, port->device->block_size);
    kprintf("%h, %h\n", port->regs[HBA_RPxIS], port->regs[HBA_RPxTFD]);
    if (!result) {
        kprintf("fail to read: %h\n", port->device->last_error);
    } else {
        kernel_log(INFO, "success: %s", buffer);
    }

    vfree(buffer);
}

void init_ext2_context(struct ext2_superblock *super_block)
{
    ext2_ctx.block_size = 1024 << super_block->s_log_block_size;
    memory_set(ext2_ctx.current_path, 0, 24);
    ext2_ctx.current_path[0] = '/';
}

void ext2_read_block(uint32_t block_index, uint8_t *buffer)
{
    struct hba_port* port = ahci_get_port(0);
    int result =
            port->device->ops.read_buffer(port, block_index, buffer, port->device->block_size);
    if (!result) {
        kernel_log(ERROR, "fail to read");
    }
}

void ext2_write_block(uint32_t block_index, uint8_t *buffer)
{
    struct hba_port* port = ahci_get_port(0);
    int result =
            port->device->ops.write_buffer(port, block_index, buffer, port->device->block_size);
    if (!result) {
        kernel_log(ERROR, "fail to write");
    }
}

void ext2_read_data(uint8_t *buffer, uint32_t block_offset, uint32_t counts)
{
    struct hba_port* port = ahci_get_port(0);
    int result = 0;

    for (uint32_t i = 0; i < counts; ++i) {
        result = port->device->ops.read_buffer(port, block_offset + i,
                                               buffer + (i*512), port->device->block_size);
        if (!result) {
            kernel_log(ERROR, "fail to read");
        }
    }
}

void ext2_write_data(uint8_t *buffer, uint32_t block_offset, uint32_t counts)
{
    struct hba_port* port = ahci_get_port(0);
    int result = 0;

    for (uint32_t i = 0; i < counts; ++i) {
        result = port->device->ops.write_buffer(port, block_offset + i,
                                               buffer + (i*512), port->device->block_size);
        if (!result) {
            kernel_log(ERROR, "fail to write");
        }
    }
}

void ext2_read_super_block(uint8_t *buffer)
{
    ext2_read_data(buffer, 2, 2);//1024 bytes
}

void ext2_read_block_group_descriptor(uint8_t *buffer)
{
    ext2_read_data(buffer, 4, 1);//sizeof(block_group_descriptor)==0x20
}

void
parse_ext2img()
{
    struct hba_port* port = ahci_get_port(0);

    kernel_log(INFO, "super block size: %h", sizeof(struct ext2_superblock));
    char *buffer = valloc(512);
    memory_set(buffer, '3', 512);
    int result;

    result = port->device->ops.read_buffer(port, 2, buffer, port->device->block_size);
    if (!result) {
        kprintf("fail to read: %h\n", port->device->last_error);
    }

    vfree(buffer);
}


