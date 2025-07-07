package com.smartirrigation.entity;
import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;
import lombok.Getter;
import lombok.Setter;

@Setter
@Getter
@Entity
@Table(name = "plant_species")
public class PlantSpecies {
    @Id
    private String species;

    private String image_url;

    // Getters and Setters
}