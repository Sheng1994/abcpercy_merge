 //test for percy link
 // main.c
#include "../cnf_gen.h"

// 声明外部的接口函数
#ifdef __cplusplus
extern "C" {
#endif
    int maintemp(void);
#ifdef __cplusplus
}
#endif

int main() {
    maintemp(); // 调用接口函数
    return 0;
}