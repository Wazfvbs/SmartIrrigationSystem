package com.smartirrigation.entity;

import jakarta.persistence.*;
import lombok.Getter;
import lombok.Setter;

@Entity
@Table(name = "user_plant", uniqueConstraints = @UniqueConstraint(columnNames = {"user_id", "plant_id"}))
public class UserPlant {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    @Getter
    @Setter
    @Column(name = "user_id")
    private Long userId;
    @Getter
    @Setter
    @Column(name = "plant_id")
    private String plantId;




    // getter & setter
}

