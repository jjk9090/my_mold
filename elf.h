
#include "integers.h"
enum {
  ELFDATA2LSB = 1,
  ELFDATA2MSB = 2,
};

enum {
  ELFCLASS32 = 1,
  ELFCLASS64 = 2,
};

enum {
  ET_NONE = 0,
  ET_REL = 1,
  ET_EXEC = 2,
  ET_DYN = 3,
};
enum  {
    EI_CLASS = 4,
    EI_DATA = 5,
    EI_VERSION = 6,
    EI_OSABI = 7,
    EI_ABIVERSION = 8,
};

enum {
  EM_NONE = 0,
  EM_386 = 3,
  EM_68K = 4,
  EM_PPC = 20,
  EM_PPC64 = 21,
  EM_S390X = 22,
  EM_ARM = 40,
  EM_SH = 42,
  EM_SPARC64 = 43,
  EM_X86_64 = 62,
  EM_AARCH64 = 183,
  EM_RISCV = 243,
  EM_LOONGARCH = 258,
  EM_ALPHA = 0x9026,
};
enum  {
  R_ARM_NONE = 0x0,
  R_ARM_PC24 = 0x1,
  R_ARM_ABS32 = 0x2,
  R_ARM_REL32 = 0x3,
  R_ARM_LDR_PC_G0 = 0x4,
  R_ARM_ABS16 = 0x5,
  R_ARM_ABS12 = 0x6,
  R_ARM_THM_ABS5 = 0x7,
  R_ARM_ABS8 = 0x8,
  R_ARM_SBREL32 = 0x9,
  R_ARM_THM_CALL = 0xa,
  R_ARM_THM_PC8 = 0xb,
  R_ARM_BREL_ADJ = 0xc,
  R_ARM_TLS_DESC = 0xd,
  R_ARM_THM_SWI8 = 0xe,
  R_ARM_XPC25 = 0xf,
  R_ARM_THM_XPC22 = 0x10,
  R_ARM_TLS_DTPMOD32 = 0x11,
  R_ARM_TLS_DTPOFF32 = 0x12,
  R_ARM_TLS_TPOFF32 = 0x13,
  R_ARM_COPY = 0x14,
  R_ARM_GLOB_DAT = 0x15,
  R_ARM_JUMP_SLOT = 0x16,
  R_ARM_RELATIVE = 0x17,
  R_ARM_GOTOFF32 = 0x18,
  R_ARM_BASE_PREL = 0x19,
  R_ARM_GOT_BREL = 0x1a,
  R_ARM_PLT32 = 0x1b,
  R_ARM_CALL = 0x1c,
  R_ARM_JUMP24 = 0x1d,
  R_ARM_THM_JUMP24 = 0x1e,
  R_ARM_BASE_ABS = 0x1f,
  R_ARM_ALU_PCREL_7_0 = 0x20,
  R_ARM_ALU_PCREL_15_8 = 0x21,
  R_ARM_ALU_PCREL_23_15 = 0x22,
  R_ARM_LDR_SBREL_11_0_NC = 0x23,
  R_ARM_ALU_SBREL_19_12_NC = 0x24,
  R_ARM_ALU_SBREL_27_20_CK = 0x25,
  R_ARM_TARGET1 = 0x26,
  R_ARM_SBREL31 = 0x27,
  R_ARM_V4BX = 0x28,
  R_ARM_TARGET2 = 0x29,
  R_ARM_PREL31 = 0x2a,
  R_ARM_MOVW_ABS_NC = 0x2b,
  R_ARM_MOVT_ABS = 0x2c,
  R_ARM_MOVW_PREL_NC = 0x2d,
  R_ARM_MOVT_PREL = 0x2e,
  R_ARM_THM_MOVW_ABS_NC = 0x2f,
  R_ARM_THM_MOVT_ABS = 0x30,
  R_ARM_THM_MOVW_PREL_NC = 0x31,
  R_ARM_THM_MOVT_PREL = 0x32,
  R_ARM_THM_JUMP19 = 0x33,
  R_ARM_THM_JUMP6 = 0x34,
  R_ARM_THM_ALU_PREL_11_0 = 0x35,
  R_ARM_THM_PC12 = 0x36,
  R_ARM_ABS32_NOI = 0x37,
  R_ARM_REL32_NOI = 0x38,
  R_ARM_ALU_PC_G0_NC = 0x39,
  R_ARM_ALU_PC_G0 = 0x3a,
  R_ARM_ALU_PC_G1_NC = 0x3b,
  R_ARM_ALU_PC_G1 = 0x3c,
  R_ARM_ALU_PC_G2 = 0x3d,
  R_ARM_LDR_PC_G1 = 0x3e,
  R_ARM_LDR_PC_G2 = 0x3f,
  R_ARM_LDRS_PC_G0 = 0x40,
  R_ARM_LDRS_PC_G1 = 0x41,
  R_ARM_LDRS_PC_G2 = 0x42,
  R_ARM_LDC_PC_G0 = 0x43,
  R_ARM_LDC_PC_G1 = 0x44,
  R_ARM_LDC_PC_G2 = 0x45,
  R_ARM_ALU_SB_G0_NC = 0x46,
  R_ARM_ALU_SB_G0 = 0x47,
  R_ARM_ALU_SB_G1_NC = 0x48,
  R_ARM_ALU_SB_G1 = 0x49,
  R_ARM_ALU_SB_G2 = 0x4a,
  R_ARM_LDR_SB_G0 = 0x4b,
  R_ARM_LDR_SB_G1 = 0x4c,
  R_ARM_LDR_SB_G2 = 0x4d,
  R_ARM_LDRS_SB_G0 = 0x4e,
  R_ARM_LDRS_SB_G1 = 0x4f,
  R_ARM_LDRS_SB_G2 = 0x50,
  R_ARM_LDC_SB_G0 = 0x51,
  R_ARM_LDC_SB_G1 = 0x52,
  R_ARM_LDC_SB_G2 = 0x53,
  R_ARM_MOVW_BREL_NC = 0x54,
  R_ARM_MOVT_BREL = 0x55,
  R_ARM_MOVW_BREL = 0x56,
  R_ARM_THM_MOVW_BREL_NC = 0x57,
  R_ARM_THM_MOVT_BREL = 0x58,
  R_ARM_THM_MOVW_BREL = 0x59,
  R_ARM_TLS_GOTDESC = 0x5a,
  R_ARM_TLS_CALL = 0x5b,
  R_ARM_TLS_DESCSEQ = 0x5c,
  R_ARM_THM_TLS_CALL = 0x5d,
  R_ARM_PLT32_ABS = 0x5e,
  R_ARM_GOT_ABS = 0x5f,
  R_ARM_GOT_PREL = 0x60,
  R_ARM_GOT_BREL12 = 0x61,
  R_ARM_GOTOFF12 = 0x62,
  R_ARM_GOTRELAX = 0x63,
  R_ARM_GNU_VTENTRY = 0x64,
  R_ARM_GNU_VTINHERIT = 0x65,
  R_ARM_THM_JUMP11 = 0x66,
  R_ARM_THM_JUMP8 = 0x67,
  R_ARM_TLS_GD32 = 0x68,
  R_ARM_TLS_LDM32 = 0x69,
  R_ARM_TLS_LDO32 = 0x6a,
  R_ARM_TLS_IE32 = 0x6b,
  R_ARM_TLS_LE32 = 0x6c,
  R_ARM_TLS_LDO12 = 0x6d,
  R_ARM_TLS_LE12 = 0x6e,
  R_ARM_TLS_IE12GP = 0x6f,
  R_ARM_PRIVATE_0 = 0x70,
  R_ARM_PRIVATE_1 = 0x71,
  R_ARM_PRIVATE_2 = 0x72,
  R_ARM_PRIVATE_3 = 0x73,
  R_ARM_PRIVATE_4 = 0x74,
  R_ARM_PRIVATE_5 = 0x75,
  R_ARM_PRIVATE_6 = 0x76,
  R_ARM_PRIVATE_7 = 0x77,
  R_ARM_PRIVATE_8 = 0x78,
  R_ARM_PRIVATE_9 = 0x79,
  R_ARM_PRIVATE_10 = 0x7a,
  R_ARM_PRIVATE_11 = 0x7b,
  R_ARM_PRIVATE_12 = 0x7c,
  R_ARM_PRIVATE_13 = 0x7d,
  R_ARM_PRIVATE_14 = 0x7e,
  R_ARM_PRIVATE_15 = 0x7f,
  R_ARM_ME_TOO = 0x80,
  R_ARM_THM_TLS_DESCSEQ16 = 0x81,
  R_ARM_THM_TLS_DESCSEQ32 = 0x82,
  R_ARM_THM_BF16 = 0x88,
  R_ARM_THM_BF12 = 0x89,
  R_ARM_THM_BF18 = 0x8a,
  R_ARM_IRELATIVE = 0xa0,
};

enum  {
  SHN_UNDEF = 0,
  SHN_LORESERVE = 0xff00,
  SHN_ABS = 0xfff1,
  SHN_COMMON = 0xfff2,
  SHN_XINDEX = 0xffff,
};

enum {
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_HASH = 5,
  SHT_DYNAMIC = 6,
  SHT_NOTE = 7,
  SHT_NOBITS = 8,
  SHT_REL = 9,
  SHT_SHLIB = 10,
  SHT_DYNSYM = 11,
  SHT_INIT_ARRAY = 14,
  SHT_FINI_ARRAY = 15,
  SHT_PREINIT_ARRAY = 16,
  SHT_GROUP = 17,
  SHT_SYMTAB_SHNDX = 18,
  SHT_RELR = 19,
  SHT_LLVM_ADDRSIG = 0x6fff4c03,
  SHT_GNU_HASH = 0x6ffffff6,
  SHT_GNU_VERDEF = 0x6ffffffd,
  SHT_GNU_VERNEED = 0x6ffffffe,
  SHT_GNU_VERSYM = 0x6fffffff,
  SHT_X86_64_UNWIND = 0x70000001,
  SHT_ARM_EXIDX = 0x70000001,
  SHT_ARM_ATTRIBUTES = 0x70000003,
  SHT_RISCV_ATTRIBUTES = 0x70000003,
};

enum  {
  SHF_WRITE = 0x1,
  SHF_ALLOC = 0x2,
  SHF_EXECINSTR = 0x4,
  SHF_MERGE = 0x10,
  SHF_STRINGS = 0x20,
  SHF_INFO_LINK = 0x40,
  SHF_LINK_ORDER = 0x80,
  SHF_GROUP = 0x200,
  SHF_TLS = 0x400,
  SHF_COMPRESSED = 0x800,
  SHF_GNU_RETAIN = 0x200000,
  SHF_EXCLUDE = 0x80000000,
};

enum  {
  VER_NDX_LOCAL = 0,
  VER_NDX_GLOBAL = 1,
  VER_NDX_LAST_RESERVED = 1,
  VER_NDX_UNSPECIFIED = 0xffff,
};

enum  {
  STT_NOTYPE = 0,
  STT_OBJECT = 1,
  STT_FUNC = 2,
  STT_SECTION = 3,
  STT_FILE = 4,
  STT_COMMON = 5,
  STT_TLS = 6,
  STT_GNU_IFUNC = 10,
  STT_SPARC_REGISTER = 13,
};

enum {
  STB_LOCAL = 0,
  STB_GLOBAL = 1,
  STB_WEAK = 2,
  STB_GNU_UNIQUE = 10,
};

enum {
  STV_DEFAULT = 0,
  STV_INTERNAL = 1,
  STV_HIDDEN = 2,
  STV_PROTECTED = 3,
};
enum {
    R_NONE = 0,
};

enum {
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6,
    PT_TLS = 7,
    PT_GNU_EH_FRAME = 0x6474e550,
    PT_GNU_STACK = 0x6474e551,
    PT_GNU_RELRO = 0x6474e552,
    PT_OPENBSD_RANDOMIZE = 0x65a3dbe6,
    PT_ARM_EXIDX = 0x70000001,
    PT_RISCV_ATTRIBUTES = 0x70000003,
};

enum {
    PF_NONE = 0,
    PF_X = 1,
    PF_W = 2,
    PF_R = 4,
};

typedef struct  {
    char *target_name;
    bool is_64;
    bool is_le;
    bool is_rela;
    u32 page_size;
    u32 e_machine;
    u32 plt_hdr_size;
    u32 plt_size;
    u32 pltgot_size;
    u32 thunk_hdr_size;
    u32 thunk_size;
    uint8_t filler[2];
    u32 R_COPY;
    u32 R_GLOB_DAT;
    u32 R_JUMP_SLOT;
    u32 R_ABS;
    u32 R_RELATIVE;
    u32 R_IRELATIVE;
    u32 R_DTPOFF;
    u32 R_TPOFF;
    u32 R_DTPMOD;
    u32 R_TLSDESC;
    u32 R_FUNCALL[5];
} ARM32;

// 定义 U32 类型
typedef struct {
    u8 val[4];
} U32;

typedef struct {
    /* data */
    u8 val[2];
} U16;

typedef struct {
    /* data */
    u8 val[4];
} U24;

// 定义 Word 类型
typedef struct {
    u8 val[4];
} Word;

typedef struct  {
    uint8_t e_ident[16];
    U16 e_type;
    U16 e_machine;
    U32 e_version;
    Word e_entry;
    Word e_phoff;
    Word e_shoff;
    U32 e_flags;
    U16 e_ehsize;
    U16 e_phentsize;
    U16 e_phnum;
    U16 e_shentsize;
    U16 e_shnum;
    U16 e_shstrndx;
} ElfEhdr;

// 定义 ElfShdr 宏

typedef struct { 
    U32 sh_name; 
    U32 sh_type; 
    Word sh_flags; 
    Word sh_addr; 
    Word sh_offset; 
    Word sh_size; 
    U32 sh_link; 
    U32 sh_info; 
    Word sh_addralign; 
    Word sh_entsize; 
} ElfShdr;

typedef struct 
{
    /* data */
    U32 st_name;
    U32 st_value;
    U32 st_size;
    u8 st_type : 4;
    u8 st_bind : 4;
    u8 st_visibility : 2;
    U16 st_shndx;
} ElfSym;

typedef struct {
    ElfShdr *begin;
    ElfShdr *end;
} Elf_Sections;


typedef struct {
    Word r_offset;
    // *r_sym.val = 3
    U24 r_sym;
    u8 r_type;
} ElfRel;

typedef struct {
    U32 p_type;
    U32 p_offset;
    U32 p_vaddr;
    U32 p_paddr;
    U32 p_filesz;
    U32 p_memsz;
    U32 p_flags;
    U32 p_align;
} ElfPhdr;