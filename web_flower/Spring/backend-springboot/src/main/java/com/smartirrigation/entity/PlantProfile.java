package com.smartirrigation.entity;

import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import lombok.Getter;
import lombok.Setter;

@Setter
@Getter
@Entity
@Table(name = "plant_profile")
public class PlantProfile {
    @Id
    private String plantId;
    private String nickname;
    private String species;

    // Getters and Setters
}
