package com.smartirrigation.controller.admin;

import com.smartirrigation.dto.BlockUserRequest;
import com.smartirrigation.entity.User;
import com.smartirrigation.service.AdminUserService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.domain.Page;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.security.access.prepost.PreAuthorize;

import java.util.Map;

@RestController
@RequestMapping("/api/admin/users")
@PreAuthorize("hasRole('ADMIN')")
public class AdminUserController {

    @Autowired
    private AdminUserService adminUserService;

    @GetMapping
    public Page<User> getUsers(
            @RequestParam int page,
            @RequestParam int size,
            @RequestParam(required = false) String keyword) {
        return adminUserService.findUsers(page, size, keyword);
    }
    @PostMapping("/block")
    public ResponseEntity<?> blockUser(@RequestBody BlockUserRequest req) {
        try {
            adminUserService.updateUserStatus(req.getUserId(), req.isBlock());
            return ResponseEntity.ok().body(Map.of("code", 200, "message", "操作成功"));
        } catch (Exception e) {
            return ResponseEntity.status(500).body(Map.of("code", 500, "message", "操作失败：" + e.getMessage()));
        }
    }
}
