#ifndef SEER_EXT2_H
#define SEER_EXT2_H
#include <stdint.h>
#include <common.h>
#include <filesystem/fs.h>

#define EXT2_SUPER_MAGIC 0xEF53
#define PHYSICAL_BLOCK_COUNTS(byte_counts) CEIL(byte_counts, 9) //512=2^9

#define EXT2_S_IFREG	0x8000	/* regular file */
#define EXT2_S_IFDIR	0x4000	/* directory */

//bad blocks inode
#define EXT2_BAD_INO	1
//root directory inode
#define EXT2_ROOT_INO	2
//ACL index inode (deprecated?)
#define EXT2_ACL_IDX_INO	3
//ACL data inode (deprecated?)
#define EXT2_ACL_DATA_INO	4
//boot loader inode
#define EXT2_BOOT_LOADER_INO	5
//undelete directory inode
#define EXT2_UNDEL_DIR_INO	6

#define VALLOC_INODE() valloc(PHYSICAL_BLOCK_SIZE * PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_inode)))
#define VALLOC_DESCRIPTOR() valloc(PHYSICAL_BLOCK_SIZE * PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_block_group_descriptor)))
#define VALLOC_DIR_ENTRY() valloc(PHYSICAL_BLOCK_SIZE * PHYSICAL_BLOCK_COUNTS(sizeof(struct ext2_directory_entry)))

struct ext2_superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    // EXT2_DYNAMIC_REV Specific
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    char s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algo_bitmap;
    // Performance Hints
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t padding1; // Alignment
    // Journaling Support
    char s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    // Directory Indexing Support
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t padding2[3]; // Reserved for future expansion
    // Other options
    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;
    uint8_t unused[760]; // Unused - reserved for future revisions
} __attribute__((packed));

struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t i_osd2[12];
} __attribute__((packed));

struct ext2_directory_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[]
} __attribute__((packed));

struct ext2_block_group_descriptor {
    uint32_t bg_block_bitmap;         // Block bitmap block
    uint32_t bg_inode_bitmap;         // Inode bitmap block
    uint32_t bg_inode_table;          // Inode table block
    uint16_t bg_free_blocks_count;    // Free blocks count
    uint16_t bg_free_inodes_count;    // Free inodes count
    uint16_t bg_used_dirs_count;      // Used directories count
    uint16_t bg_pad;                  // Padding to word
    uint8_t  bg_reserved[12];         // Reserved space
} __attribute__((packed));

// Read the Superblock to find the size of each block,
// the number of blocks per group, number Inodes per group,
// and the starting block of the first group (Block Group Descriptor Table).
struct ext2_context {
    char current_path[24];
    uint32_t block_size;
    uint32_t s_blocks_per_group;
    uint32_t s_inodes_per_group;
//    uint32_t block_of_first_group;
    uint32_t group_counts;
    uint16_t s_inode_size;

    // Block Group Descriptor
    uint32_t root_inode_table;
    uint16_t bg_used_dirs_count;
} __attribute__((packed));

void init_ext2_context(struct ext2_superblock *, struct ext2_block_group_descriptor *);

// block offset: 0 -> 0*512, 1 -> 1*512
// counts: 1 -> 1*512, 2 -> 2*512
void ext2_read_data(uint8_t *buffer, uint32_t block_offset, uint32_t counts);

void ext2_write_data(uint8_t *buffer, uint32_t block_offset, uint32_t counts);

void ext2_read_super_block(uint8_t *buffer);

void ext2_app();

struct ext2_inode *ext2_inode_get(char *path);

uint32_t find_block_group_index(uint32_t inode);

uint32_t find_local_inode_index(uint32_t inode);

uint32_t find_inode_table_block_offset(uint32_t group_index);

uint32_t find_block_bitmap_block_offset(uint32_t group_index);

uint32_t find_inode_bitmap_block_offset(uint32_t group_index);

uint32_t find_first_block_block_offset(uint32_t group_index);

#endif