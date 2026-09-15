#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>

static struct kprobe kp_porch = {
    .symbol_name = "mtk_dsi_porch_str",
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    pr_info("mtk_display_oc: kprobe hit on mtk_dsi_porch_str!\n");
    return 0;
}

static int __init oc_init(void) {
    kp_porch.pre_handler = handler_pre;
    int ret = register_kprobe(&kp_porch);
    if (ret < 0) {
        pr_err("mtk_display_oc: register_kprobe failed, error %d\n", ret);
        return ret;
    }
    pr_info("mtk_display_oc: module loaded, kprobe attached\n");
    return 0;
}

static void __exit oc_exit(void) {
    unregister_kprobe(&kp_porch);
    pr_info("mtk_display_oc: module unloaded\n");
}

module_init(oc_init);
module_exit(oc_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTK Display Overclock for Realme Neo7 Turbo");
