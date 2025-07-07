package com.smartirrigation.controller;

import com.smartirrigation.entity.User;
import com.smartirrigation.repository.UserRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.security.Principal;
import java.util.Map;
import java.util.Optional;

@RestController
@RequestMapping("/api/user")
public class UserController {

    @Autowired
    private UserRepository userRepository; // 👈 注入 Repository

    @GetMapping("/info")
    public ResponseEntity<?> getUserInfo(Principal principal) {
        Optional<User> optionalUser = userRepository.findByUsername(principal.getName());
        if (optionalUser.isPresent()) {
            User user = optionalUser.get();
            return ResponseEntity.ok(Map.of(
                    "username", user.getUsername(),
                    "nickname", user.getNickname()
            ));
        } else {
            return ResponseEntity.status(404).body(Map.of("msg", "用户不存在"));
        }
    }
}
