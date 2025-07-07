package com.smartirrigation.entity;

import jakarta.persistence.*;
import lombok.Data;

import java.time.LocalDateTime;

@Entity
@Table(name = "plant_data", uniqueConstraints = @UniqueConstraint(columnNames = {"plant_id", "timestamp"}))
@Data
public class PlantData {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private String plantId;

    private LocalDateTime timestamp;

    private Double temperature;
    private Double humidity;
    private Double soil_moisture;
    private Integer light;
    private Double water_level;
    private Integer battery;
    private String mode;
}
