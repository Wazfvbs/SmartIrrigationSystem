package com.smartirrigation.repository;

import com.smartirrigation.entity.User;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Optional;

public interface UserRepository extends JpaRepository<User, Long> {
    Optional<User> findByUsername(String username);

    // ✅ 新增：分页+模糊查询
    Page<User> findByUsernameContainingIgnoreCase(String keyword, Pageable pageable);
}
