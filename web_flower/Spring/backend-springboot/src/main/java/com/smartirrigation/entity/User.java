package com.smartirrigation.entity;

import jakarta.persistence.*;
import lombok.Data;
import lombok.Getter;
import lombok.Setter;

@Entity
@Table(name = "users")
@Data
public class User {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false, unique = true)
    private String username;

    @Column(nullable = false)
    private String password;

    @Column
    private String nickname;  // 👈 新增字段

    @Setter
    @Getter
    @Column(name = "is_blocked")
    private boolean blocked;

    @Setter
    @Getter
    @Column(nullable = false)
    private String role = "USER";
}
