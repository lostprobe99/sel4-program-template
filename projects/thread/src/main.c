#include <stdio.h>
#include <sel4/sel4.h>
#include <sel4platsupport/bootinfo.h>
#include <utils/util.h>
#include <sel4utils/util.h>
#include <sel4utils/helpers.h>

/* stack for the new thread */
#define THREAD_2_STACK_SIZE 512
static uint64_t thread_2_stack[THREAD_2_STACK_SIZE];
int child_thread_state;

void thread_2(uint64_t * stack_top)
{
    printf("stack_top = %#x\n", stack_top);
    printf("thread_2 starting.\n");
    uint64_t n = 0xff;
    printf("n = %#x\n", n);
    uint64_t nums[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint64_t * it = stack_top;
    for(int i = 0; i < 4; i++)
    {
        printf("stack[%#x]: ", stack_top - i * 8);
        for(int j = 0; j < 8; j++)
            printf("%-#16x ", *(stack_top - (8 * i + j)));
        printf("\n");
    }
    printf("stack bottom[thread_2_stack] = %#x\n", thread_2_stack);
    seL4_DebugDumpScheduler();
    printf("\n\n\n");
    child_thread_state = 0;
}

int main(int argc, char *argv[]) {
    // Step 1.获取 BootInfo
    seL4_BootInfo *info = platsupport_get_bootinfo();
    seL4_Error error;
    seL4_CPtr parent_untyped = 0;	// 可用 untyped 的 cap
    seL4_CPtr child_tcb = info->empty.start;    // tcb 的 cap

    // Step 2.寻找一块足够大的 untyped 空间
    for (int i = 0; i < (info->untyped.end - info->untyped.start); i++) {
        if (info->untypedList[i].sizeBits >= seL4_TCBBits && !info->untypedList[i].isDevice) {
            parent_untyped = info->untyped.start + i;
            printf("parent_untyped = %#lx\n", parent_untyped);
            break;
        }
    }

	// Step 3.创建 TCB 对象
    // seL4_TCBObject 有固定大小，第三个参数可填 0 或 seL4_TCBBits
    error = seL4_Untyped_Retype(parent_untyped, seL4_TCBObject, 0, seL4_CapInitThreadCNode, 0, 0, child_tcb, 1);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to create tcb");
    printf("child_tcb = %#lx\n", child_tcb);

    seL4_DebugNameThread(child_tcb, "thread_2");
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "thread_main");
    // Step 4.初始化 tcb 对象
    error = seL4_TCB_Configure(child_tcb, seL4_CapNull,
                               seL4_CapInitThreadCNode, seL4_NilData,
                               seL4_CapInitThreadVSpace, seL4_NilData,
                               NULL, NULL);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to configure tcb");
	// Step 5.设置线程优先级为255
    error = seL4_TCB_SetPriority(child_tcb, seL4_CapInitThreadTCB, 255);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to set priority");
	// Step 6.为新线程创建上下文对象
    seL4_UserContext regs = {0};
    seL4_Word num_regs = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    // regs.rip = (seL4_Word)thread_2;	// 新线程执行的指令的起始地址
    // sel4utils_set_instruction_pointer(&regs, thread_2);

    // 检查栈是否16字节对齐
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t thread_2_stack_top = (uintptr_t)thread_2_stack + sizeof(thread_2_stack);
    ZF_LOGF_IF(thread_2_stack_top % (stack_alignment_requirement) != 0,
               "Stack top isn't aligned correctly to a %dB boundary.\n",
               stack_alignment_requirement);
    printf("new thread stack top = %#x\n", thread_2_stack_top);

    // regs.rsp = (seL4_Word)thread_2_stack_top;	// 新线程的运行栈
    // sel4utils_set_stack_pointer(&regs, thread_2_stack_top);

    // Step 7.设置新线程的入口点，运行栈，参数
    sel4utils_arch_init_local_context(thread_2,	// 线程入口点
					thread_2_stack_top, 2, 3,   //  三个参数
					thread_2_stack_top, &regs); // 栈顶指针和寄存器
	// Step 8.将刚刚创建的寄存器信息写入线程
    error = seL4_TCB_WriteRegisters(child_tcb, 0, 0, num_regs, &regs);

    printf("main: hello world\n");

    seL4_DebugDumpScheduler();
    child_thread_state = 1;
    // Step 9.启动线程
    error = seL4_TCB_Resume(child_tcb);
    ZF_LOGF_IF(error != seL4_NoError, "Failed to resume tcb");
    printf("子线程已启动\n");

    while(child_thread_state == 1);

    printf("子线程 [%#x] 已结束\n", child_tcb);
    seL4_DebugDumpScheduler();

    printf("\n\n\n");
    while(1);

    return 0;
}