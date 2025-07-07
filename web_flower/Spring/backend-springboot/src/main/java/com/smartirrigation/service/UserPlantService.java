package com.smartirrigation.service;

import com.smartirrigation.dto.PlantInfoDTO;
import com.smartirrigation.entity.UserPlant;
import com.smartirrigation.repository.UserPlantRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.stereotype.Service;

import java.util.List;
import java.util.stream.Collectors;

@Service
public class UserPlantService {
    @Autowired
    private UserPlantRepository userPlantRepository;

    public List<String> getPlantIdsByUserId(Long userId) {
        return userPlantRepository.findByUserId(userId).stream()
                .map(UserPlant::getPlantId)
                .collect(Collectors.toList());
    }

    public void bindPlant(Long userId, String plantId) {
        UserPlant userPlant = new UserPlant();
        userPlant.setUserId(userId);
        userPlant.setPlantId(plantId);
        userPlantRepository.save(userPlant);
    }
    @Autowired
    private JdbcTemplate jdbcTemplate;

    public List<PlantInfoDTO> getPlantsByUser(Long userId) {
        String sql = """
        SELECT up.plant_id, '植物' || up.plant_id AS nickname, 
               ps.species, ps.image_url
        FROM user_plant up
        LEFT JOIN plant_species ps ON up.plant_id = ps.species
        WHERE up.user_id = ?
    """;

        return jdbcTemplate.query(sql, new Object[]{userId}, (rs, rowNum) -> {
            PlantInfoDTO dto = new PlantInfoDTO();
            dto.setPlantId(rs.getString("plant_id"));
            dto.setNickname(rs.getString("nickname"));
            dto.setSpecies(rs.getString("species"));
            dto.setImageUrl(rs.getString("image_url"));
            return dto;
        });
    }
}
