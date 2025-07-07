package com.smartirrigation.dto;

import lombok.Data;

@Data
public class PlantDataDTO {
    private String plant_id;
    private String timestamp;
    private Double temperature;
    private Double humidity;
    private Double soil_moisture;
    private Integer light;
    private Double water_level;
    private Integer battery;
    private String mode;
}
