package com.smartirrigation.dto;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@AllArgsConstructor
@NoArgsConstructor
public class PlantInfoDTO {
    private String plantId;
    private String nickname;
    private String species;
    private String imageUrl;
}
