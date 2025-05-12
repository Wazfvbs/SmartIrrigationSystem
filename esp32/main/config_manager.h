#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>

typedef struct
{
    char device_name[32];
    char species[32];
    double threshold_upper;
    double threshold_lower;
} device_config_t;

void config_manager_init(void);

// 更新设备昵称
void config_manager_update_device_name(const char *name);

// 更新植物种类
void config_manager_update_species(const char *species);

// 更新浇水上下阈值
void config_manager_update_threshold(double upper, double lower);

// 获取当前配置（返回指针，只读）
const device_config_t *config_manager_get_config(void);

#endif // CONFIG_MANAGER_H
