package com.smartirrigation.controller;

import com.smartirrigation.entity.User;
import com.smartirrigation.repository.UserRepository;
import com.smartirrigation.util.JwtUtil;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@RequestMapping("/api/auth")
public class AuthController {

    @Autowired
    private UserRepository userRepository;

    @Autowired
    private PasswordEncoder passwordEncoder;

    @Autowired
    private JwtUtil jwtUtil;

    // ✅ 注册接口
    @PostMapping("/register")
    public ResponseEntity<?> register(@RequestBody User user) {
        user.setPassword(passwordEncoder.encode(user.getPassword()));
        userRepository.save(user);
        return ResponseEntity.ok(Map.of("msg", "注册成功"));
    }

    // ✅ 登录接口（带上 role）
    @PostMapping("/login")
    public ResponseEntity<?> login(@RequestBody User user) {
        return userRepository.findByUsername(user.getUsername())
                .filter(u -> passwordEncoder.matches(user.getPassword(), u.getPassword()))
                .map(u -> {
                    // 确保此处从数据库取出角色
                    String role = u.getRole(); // 👈确保这个值是 ADMIN
                    String token = jwtUtil.generateToken(u.getId(), u.getUsername(), u.getRole());

                    return ResponseEntity.ok(Map.of(
                            "token", token,
                            "user", Map.of(
                                    "username", u.getUsername(),
                                    "role", role
                            )
                    ));
                })
                .orElse(ResponseEntity.status(401).body(Map.of("msg", "用户名或密码错误")));
    }

}
