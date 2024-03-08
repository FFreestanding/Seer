#include <filesystem/ext2.h>
#include <kernel_io/vga.h>

struct ext2_superblock superblocks;

void parse_superblock(struct ext2_superblock *super_block_ptr)
{
  
    superblocks = *super_block_ptr;

    if (superblocks.s_magic == EXT2_SUPER_MAGIC)
    {
        kernel_log(INFO, "It is ext2 filesystem. Magic Number: 0x%h\n", 
            superblocks.s_magic);
    }
    else
    {
        kernel_log(INFO, "[WARNNING] It is not ext2 filesystem. Magic Number: 0x%h\n", 
            superblocks.s_magic);
    }

    kernel_log(INFO, "s_volume_name:%s\n"
            "s_last_mounted:%s\n"
            "s_first_data_block:%h\n"
            "block_size:%uB\n"
            "s_feature_ro_compat:%h\n", 
            superblocks.s_volume_name, 
            superblocks.s_last_mounted,
            superblocks.s_first_data_block,
            (1024 << superblocks.s_log_block_size),
            superblocks.s_feature_ro_compat);
}
