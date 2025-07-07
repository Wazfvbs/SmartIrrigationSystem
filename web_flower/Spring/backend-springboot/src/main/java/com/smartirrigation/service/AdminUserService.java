package com.smartirrigation.service;

import com.smartirrigation.entity.User;
import org.springframework.data.domain.Page;

public interface AdminUserService {
    Page<User> findUsers(int page, int size, String keyword);
    void updateUserStatus(Long userId, boolean block);
}
