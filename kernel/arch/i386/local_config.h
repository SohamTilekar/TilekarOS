// The index in the GDT where the TSS descriptor will be placed.
#define GDT_KERNEL_CS_INDEX 1
#define GDT_KERNEL_DS_INDEX 2
#define GDT_USER_CS_INDEX 3
#define GDT_USER_DS_INDEX 4
#define GDT_TSS_INDEX 5

#define GDT_KERNEL_CS_OFFSET (GDT_KERNEL_CS_INDEX * 8) | 0
#define GDT_KERNEL_DS_OFFSET (GDT_KERNEL_DS_INDEX * 8) | 0
#define GDT_USER_CS_OFFSET (GDT_USER_CS_INDEX * 8) | 0
#define GDT_USER_DS_OFFSET (GDT_USER_DS_INDEX * 8) | 0
#define GDT_TSS_OFFSET (GDT_TSS_INDEX * 8) | 0
