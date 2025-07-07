package com.smartirrigation.service.impl;

import com.smartirrigation.entity.User;
import com.smartirrigation.repository.UserRepository;
import com.smartirrigation.service.AdminUserService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.domain.*;
import org.springframework.stereotype.Service;

@Service
public class AdminUserServiceImpl implements AdminUserService {

    @Autowired
    private UserRepository userRepository;

    @Override
    public Page<User> findUsers(int page, int size, String keyword) {
        Pageable pageable = PageRequest.of(page, size, Sort.by("id").descending());
        if (keyword == null || keyword.isBlank()) {
            return userRepository.findAll(pageable);
        }
        return userRepository.findByUsernameContainingIgnoreCase(keyword, pageable);
    }
    @Override
    public void updateUserStatus(Long userId, boolean block) {
        User user = userRepository.findById(userId)
                .orElseThrow(() -> new IllegalArgumentException("用户不存在"));
        user.setBlocked(block);
        userRepository.save(user);
    }
}
