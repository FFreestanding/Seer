#ifndef SEER_EXT2_H
#define SEER_EXT2_H
#include <stdint.h>

#define EXT2_SUPER_MAGIC 0xEF53

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
    // uint8_t unused[760]; // Unused - reserved for future revisions
};

// Inode i_osd2 Structure
// 96bit OS dependant structure.
// Table 3.18. Inode i_osd2 Structure: Linux
struct ext2_inode {
    uint8_t l_i_frag;       // Fragment number
    uint8_t l_i_fsize;      // Fragment size
    uint16_t reserved1;     // Reserved (used for alignment and future expansion)
    uint16_t l_i_uid_high;  // High 16-bits of the Owner UID
    uint16_t l_i_gid_high;  // High 16-bits of the Owner GID
    // uint32_t reserved2;     // Reserved (more alignment and future expansion)
};

struct ext2_block_group_descriptor_table {
    uint32_t bg_block_bitmap;         // Block bitmap block
    uint32_t bg_inode_bitmap;         // Inode bitmap block
    uint32_t bg_inode_table;          // Inode table block
    uint16_t bg_free_blocks_count;    // Free blocks count
    uint16_t bg_free_inodes_count;    // Free inodes count
    uint16_t bg_used_dirs_count;      // Used directories count
    uint16_t bg_pad;                  // Padding to word
    uint8_t  bg_reserved[12];         // Reserved space
} __attribute__((packed));

#endif