package com.smartirrigation.controller;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/test")
public class HelloController {
    @GetMapping("/ping")
    public String ping() {
        return "pong";
    }
}
