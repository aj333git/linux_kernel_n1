// ptr_demo.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Demo");
MODULE_DESCRIPTION("Demonstrate unsigned char* versus u16*");
MODULE_VERSION("1.0");

static int __init ptr_demo_init(void)
{
    /*
     * Four bytes of raw data.
     * The u16 pointer below is suitably aligned because
     * the buffer is explicitly aligned to 2 bytes.
     */
    static const unsigned char buffer[4] __aligned(2) = {
        0x12, 0x34, 0x56, 0x78
    };

    const unsigned char *byte_ptr = buffer;
    const u16 *word_ptr = (const u16 *)buffer;

    pr_info("ptr_demo: module loaded\n");

    /* Demonstrate byte-by-byte access. */
    pr_info("ptr_demo: byte_ptr[0] = 0x%02x\n",
            (unsigned int)byte_ptr[0]);
    pr_info("ptr_demo: byte_ptr[1] = 0x%02x\n",
            (unsigned int)byte_ptr[1]);
    pr_info("ptr_demo: byte_ptr[2] = 0x%02x\n",
            (unsigned int)byte_ptr[2]);
    pr_info("ptr_demo: byte_ptr[3] = 0x%02x\n",
            (unsigned int)byte_ptr[3]);

    /* Demonstrate 16-bit access. */
    pr_info("ptr_demo: word_ptr[0] = 0x%04x\n",
            (unsigned int)word_ptr[0]);
    pr_info("ptr_demo: word_ptr[1] = 0x%04x\n",
            (unsigned int)word_ptr[1]);

    /* Demonstrate pointer arithmetic. */
    pr_info("ptr_demo: byte_ptr + 1 advances %zu byte\n",
            (size_t)((const unsigned char *)(byte_ptr + 1) -
                     (const unsigned char *)byte_ptr));

    pr_info("ptr_demo: word_ptr + 1 advances %zu bytes\n",
            (size_t)((const unsigned char *)(word_ptr + 1) -
                     (const unsigned char *)word_ptr));

    return 0;
}

static void __exit ptr_demo_exit(void)
{
    pr_info("ptr_demo: module unloaded\n");
}

module_init(ptr_demo_init);
module_exit(ptr_demo_exit);

