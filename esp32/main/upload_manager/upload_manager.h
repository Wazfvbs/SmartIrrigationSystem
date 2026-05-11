#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

void upload_manager_start(void);

// 更新待上传的最新JSON数据（UART任务收到新数据后调用）
void upload_manager_update_data(const char *json_data);

#endif // UPLOAD_MANAGER_H
